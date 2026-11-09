// PESMotion - pesmotion(1): CLI principal.
// Uso: pesmotion <comando> [opções]
//   selftest                       Roda a bateria de verificações internas
//   profiles                       Lista os perfis PES 2013–2021
//   info <arquivo.(bvh|pmca)>      Inspeciona o arquivo
//   convert --in x.bvh --out y.pmca [--reduce-angle GRAUS] [--reduce-root METROS]
//                                  [--scale FATOR] [--in-place]
//   build --profile pes2021 --out PesMotion_PES2021.bin (<clipe.pmca>... | --clips-dir DIR)
//   install --bin <arquivo.bin> --dest <pasta>   Copia o build para a pasta do jogo

#include "math3d.h"
#include "bvh.h"
#include "pmca.h"
#include "pesbin.h"
#include "retarget.h"
#include "skeleton.h"
#include "selftest.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#endif

namespace {

constexpr const char* kVersion = PESMOTION_VERSION;

void printUsage() {
    std::printf(
        "PESMotion %s - Hyper Motion ultra-leve para PES 2013-2021\n"
        "Uso:\n"
        "  pesmotion selftest\n"
        "  pesmotion profiles\n"
        "  pesmotion info <arquivo.(bvh|pmca)>\n"
        "  pesmotion convert --in <x.bvh> --out <y.pmca> [--reduce-angle G] [--reduce-root M]\n"
        "                    [--scale F] [--in-place]\n"
        "  pesmotion build --profile <pes2013..pes2021> --out <saida.bin> (<clipe.pmca>... | --clips-dir DIR)\n"
        "  pesmotion install --bin <arquivo.bin> --dest <pasta-do-jogo>\n",
        kVersion);
}

bool fileExists(const std::string& path) {
    struct stat st{};
    return stat(path.c_str(), &st) == 0;
}

bool ensureDir(const std::string& path) {
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0 || fileExists(path);
#else
    return mkdir(path.c_str(), 0755) == 0 || fileExists(path);
#endif
}

std::vector<std::string> listPmcaFiles(const std::string& dir) {
    // Lista simples e portável via glob em shells; aqui: varre com opendir POSIX ou _wfindfirst.
    std::vector<std::string> out;
#ifdef _WIN32
    WIN32_FIND_DATAA fd{};
    std::string pattern = dir + "\\*.pmca";
    HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do { out.push_back(dir + "\\" + fd.cFileName); } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR* d = opendir(dir.c_str());
    if (d) {
        while (auto* e = readdir(d)) {
            std::string n = e->d_name;
            if (n.size() > 5 && n.compare(n.size() - 5, 5, ".pmca") == 0) out.push_back(dir + "/" + n);
        }
        closedir(d);
    }
#endif
    return out;
}

std::string baseNameNoExt(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    std::string file = (slash == std::string::npos) ? path : path.substr(slash + 1);
    size_t dot = file.find_last_of('.');
    return (dot == std::string::npos) ? file : file.substr(0, dot);
}

int cmdProfiles() {
    std::printf("Perfis PESMotion (2013-2021):\n");
    for (const auto& p : pm::pesProfiles()) {
        std::printf("  %-9s %-28s formato=%u  header=%uB  status=%s\n",
                    p.id, p.gameTitle, p.formatVersion, p.keyframeHeaderBytes,
                    p.experimental ? "experimental (RE pendente)" : "estavel");
    }
    return 0;
}

int cmdInfo(const std::string& path) {
    // Detecta por extensão; BVH é texto, PMCA é binário.
    if (path.size() >= 4 && path.compare(path.size() - 4, 4, ".bvh") == 0) {
        pm::BvhAnim anim; std::string err;
        if (!pm::loadBvhFile(path, anim, err)) { std::fprintf(stderr, "erro: %s\n", err.c_str()); return 1; }
        size_t chans = 0;
        for (const auto& b : anim.boneInfo) chans += b.channels.size();
        std::printf("BVH: %d ossos, %d frames, dt=%.5fs (%.1f fps), %zu canais\n",
                    anim.skeleton.boneCount(), anim.frameCount, anim.dt, 1.0f / anim.dt, chans);
        for (int i = 0; i < anim.skeleton.boneCount(); ++i) {
            const auto& b = anim.skeleton.bones[i];
            std::printf("  [%2d] %-24s pai=%2d offset=(%.3f, %.3f, %.3f) canais=%zu\n",
                        i, b.name.c_str(), b.parent, b.offset.x, b.offset.y, b.offset.z,
                        anim.boneInfo[i].channels.size());
        }
        return 0;
    }
    if (path.size() >= 5 && path.compare(path.size() - 5, 5, ".pmca") == 0) {
        pm::AnimClip clip; std::string err;
        if (!pm::readPmcaFile(path, clip, err)) { std::fprintf(stderr, "erro: %s\n", err.c_str()); return 1; }
        std::printf("PMCA: clip '%s', %d ossos, %d frames, dt=%.5fs (%.1f fps)\n",
                    clip.name.c_str(), clip.boneCount, clip.frameCount, clip.dt, 1.0f / clip.dt);
        return 0;
    }
    std::fprintf(stderr, "extensão não suportada (use .bvh ou .pmca)\n");
    return 1;
}

int cmdConvert(int argc, char** argv) {
    std::string inPath, outPath;
    float reduceAngleDeg = -1.0f;   // negativo = não reduzir
    float reduceRoot = -1.0f;
    float scale = 1.0f;
    bool inPlace = false;
    for (int i = 0; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "falta valor para %s\n", flag); std::exit(1); }
            return argv[++i];
        };
        if (a == "--in") inPath = need("--in");
        else if (a == "--out") outPath = need("--out");
        else if (a == "--reduce-angle") reduceAngleDeg = std::stof(need("--reduce-angle"));
        else if (a == "--reduce-root") reduceRoot = std::stof(need("--reduce-root"));
        else if (a == "--scale") scale = std::stof(need("--scale"));
        else if (a == "--in-place") inPlace = true;
        else { std::fprintf(stderr, "opção desconhecida: %s\n", a.c_str()); return 1; }
    }
    if (inPath.empty() || outPath.empty()) { std::fprintf(stderr, "uso: pesmotion convert --in x.bvh --out y.pmca\n"); return 1; }

    pm::BvhAnim anim; std::string err;
    if (!pm::loadBvhFile(inPath, anim, err)) { std::fprintf(stderr, "erro: %s\n", err.c_str()); return 1; }

    pm::RetargetOptions opt;
    opt.rootTranslationScale = scale;
    opt.zeroRootMotion = inPlace;

    std::string clipName = baseNameNoExt(inPath);
    std::string rerr;
    pm::AnimClip clip = pm::convertBvhToClip(anim, clipName, opt, rerr);
    if (clip.frameCount <= 0) { std::fprintf(stderr, "erro no retarget: %s\n", rerr.c_str()); return 1; }

    int removed = 0;
    if (reduceAngleDeg >= 0.0f || reduceRoot >= 0.0f) {
        float ang = reduceAngleDeg >= 0.0f ? pm::degToRad(reduceAngleDeg) : pm::degToRad(0.5f);
        float root = reduceRoot >= 0.0f ? reduceRoot : 0.01f;
        removed = pm::reduceAnimClip(clip, ang, root);
    }

    std::string werr;
    if (!pm::writePmcaFile(clip, outPath, werr)) { std::fprintf(stderr, "erro: %s\n", werr.c_str()); return 1; }
    std::printf("convertido: %s -> %s (%d frames%s, %d ossos)\n",
                inPath.c_str(), outPath.c_str(), clip.frameCount,
                removed > 0 ? (", redução removeu " + std::to_string(removed)).c_str() : "",
                clip.boneCount);
    return 0;
}

int cmdBuild(int argc, char** argv) {
    std::string profileId, outPath, clipsDir;
    std::vector<std::string> clipFiles;
    for (int i = 0; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "falta valor para %s\n", flag); std::exit(1); }
            return argv[++i];
        };
        if (a == "--profile") profileId = need("--profile");
        else if (a == "--out") outPath = need("--out");
        else if (a == "--clips-dir") clipsDir = need("--clips-dir");
        else if (a.rfind("--", 0) == 0) { std::fprintf(stderr, "opção desconhecida: %s\n", a.c_str()); return 1; }
        else clipFiles.push_back(a);
    }
    if (profileId.empty() || outPath.empty()) {
        std::fprintf(stderr, "uso: pesmotion build --profile pes2021 --out out.bin [clipes.pmca...|--clips-dir DIR]\n");
        return 1;
    }
    const pm::PesProfile* profile = pm::findPesProfile(profileId);
    if (!profile) { std::fprintf(stderr, "perfil desconhecido: %s (use `pesmotion profiles`)\n", profileId.c_str()); return 1; }

    if (!clipsDir.empty()) {
        auto found = listPmcaFiles(clipsDir);
        clipFiles.insert(clipFiles.end(), found.begin(), found.end());
    }
    if (clipFiles.empty()) { std::fprintf(stderr, "nenhum clip informado\n"); return 1; }

    std::vector<pm::AnimClip> clips;
    clips.reserve(clipFiles.size());
    for (const auto& f : clipFiles) {
        pm::AnimClip c; std::string err;
        if (!pm::readPmcaFile(f, c, err)) { std::fprintf(stderr, "erro lendo %s: %s\n", f.c_str(), err.c_str()); return 1; }
        clips.push_back(std::move(c));
    }

    std::string err;
    if (!pm::buildPesBinFile(*profile, clips, outPath, err)) { std::fprintf(stderr, "erro: %s\n", err.c_str()); return 1; }
    unsigned long totalFrames = 0;
    for (const auto& c : clips) totalFrames += static_cast<unsigned long>(c.frameCount);
    std::printf("build concluído: %s (perfil %s, %zu clipes, %lu frames)\n",
                outPath.c_str(), profile->id, clips.size(), totalFrames);
    if (profile->experimental) {
        std::printf("aviso: perfil %s é EXPERIMENTAL — valide em campo antes de distribuir (docs/FORMATS.md).\n",
                    profile->id);
    }
    return 0;
}

int cmdInstall(int argc, char** argv) {
    std::string binPath, dest;
    for (int i = 0; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "falta valor para %s\n", flag); std::exit(1); }
            return argv[++i];
        };
        if (a == "--bin") binPath = need("--bin");
        else if (a == "--dest") dest = need("--dest");
        else { std::fprintf(stderr, "opção desconhecida: %s\n", a.c_str()); return 1; }
    }
    if (binPath.empty() || dest.empty()) { std::fprintf(stderr, "uso: pesmotion install --bin out.bin --dest <pasta>\n"); return 1; }
    if (!fileExists(binPath)) { std::fprintf(stderr, "arquivo não encontrado: %s\n", binPath.c_str()); return 1; }
    if (!ensureDir(dest)) { std::fprintf(stderr, "não foi possível criar/usar a pasta: %s\n", dest.c_str()); return 1; }

    std::ifstream src(binPath, std::ios::binary);
    std::string destFile = dest + "/PesMotion.bin";
    std::ofstream dst(destFile, std::ios::binary | std::ios::trunc);
    dst << src.rdbuf();
    if (!dst) { std::fprintf(stderr, "falha ao copiar para %s\n", destFile.c_str()); return 1; }

    // Manifesto de instalação (auditável).
    std::time_t now = std::time(nullptr);
    std::ofstream manifest(dest + "/PesMotion.manifest.txt", std::ios::trunc);
    manifest << "PESMotion install manifest\n";
    manifest << "source: " << binPath << "\n";
    manifest << "installed_at: " << static_cast<long long>(now) << " (unix time)\n";
    manifest << "tool_version: " << kVersion << "\n";
    manifest << "note: perfis de jogo são experimentais; valide em campo (docs/FORMATS.md)\n";

    std::printf("instalado: %s\n", destFile.c_str());
    std::printf("manifesto: %s/PesMotion.manifest.txt\n", dest.c_str());
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) { printUsage(); return 1; }
    std::string cmd = argv[1];
    if (cmd == "selftest") return pm::runSelfTests() == 0 ? 0 : 1;
    if (cmd == "profiles") return cmdProfiles();
    if (cmd == "info") { if (argc < 3) { std::fprintf(stderr, "uso: pesmotion info <arquivo>\n"); return 1; } return cmdInfo(argv[2]); }
    if (cmd == "convert") return cmdConvert(argc - 2, argv + 2);
    if (cmd == "build") return cmdBuild(argc - 2, argv + 2);
    if (cmd == "install") return cmdInstall(argc - 2, argv + 2);
    if (cmd == "help" || cmd == "--help" || cmd == "-h") { printUsage(); return 0; }
    if (cmd == "version" || cmd == "--version") { std::printf("pesmotion %s\n", kVersion); return 0; }
    std::fprintf(stderr, "comando desconhecido: %s\n\n", cmd.c_str());
    printUsage();
    return 1;
}
