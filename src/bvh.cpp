#include "bvh.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstring>

namespace pm {

// ---------- Tokenizer ----------
struct BvhTokenizer {
    std::string src;
    size_t pos = 0;
};

static std::vector<std::string> tokenize(const std::string& src) {
    std::vector<std::string> out;
    size_t i = 0, n = src.size();
    auto isSpace = [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; };
    while (i < n) {
        char c = src[i];
        if (isSpace(c)) { ++i; continue; }
        if (c == '{' || c == '}') { out.emplace_back(1, c); ++i; continue; }
        size_t j = i;
        while (j < n && !isSpace(src[j]) && src[j] != '{' && src[j] != '}') ++j;
        out.emplace_back(src.substr(i, j - i));
        i = j;
    }
    return out;
}

// ---------- Parser ----------
struct BvhParser {
    std::vector<std::string> tok;
    size_t i = 0;
    BvhAnim& anim;
    std::string& error;
    int valueIndex = 0;

    BvhParser(std::vector<std::string> t, BvhAnim& a, std::string& e)
        : tok(std::move(t)), anim(a), error(e) {}

    bool fail(const std::string& msg) { error = msg; return false; }
    const std::string& peek() const {
        static const std::string kEmpty;
        return have() ? tok[i] : kEmpty;
    }
    bool have() const { return i < tok.size(); }
    std::string next() { return tok[i++]; }
    bool expect(const std::string& what, std::string& got) {
        if (!have()) return fail("fim inesperado; esperava " + what);
        got = tok[i];
        if (got != what) return fail("esperava '" + what + "', encontrei '" + got + "'");
        ++i;
        return true;
    }
    float nextFloat(const std::string& ctx) {
        if (!have()) { fail("fim inesperado lendo float em " + ctx); return 0.0f; }
        std::string t = next();
        try { return std::stof(t); } catch (...) { fail("float inválido '" + t + "' em " + ctx); return 0.0f; }
    }
    int nextInt(const std::string& ctx) {
        if (!have()) { fail("fim inesperado lendo inteiro em " + ctx); return 0; }
        std::string t = next();
        try { return std::stoi(t); } catch (...) { fail("inteiro inválido '" + t + "' em " + ctx); return 0; }
    }

    // Normaliza "Frames:" -> "Frames:" (sem conteúdo extra) e tudo para minúsculo p/ comparação.
    static std::string normKey(const std::string& t) {
        std::string s = t;
        while (!s.empty() && s.back() == ':') s.pop_back();
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    bool parse() {
        // Hierrarquia
        if (!have() || normKey(peek()) != "hierarchy") return fail("arquivo não começa com HIERARCHY");
        ++i;
        int rootParent = -1;
        if (!parseNode(rootParent, true)) return false;

        // Motion
        if (!have() || normKey(peek()) != "motion") return fail("seção MOTION ausente");
        ++i;
        // Frames:
        if (normKey(peek()) != "frames") return fail("esperava 'Frames:'");
        ++i;
        anim.frameCount = nextInt("Frames:");
        if (anim.frameCount <= 0) return fail("Frames: inválido");
        // "Frame Time:" pode chegar tokenizado como "Frame" "Time:" ou "FrameTime:"
        {
            std::string k1 = normKey(peek());
            if (k1 == "frame") {
                ++i; // "Frame"
                if (!have() || normKey(peek()) != "time") return fail("esperava 'Time:' após 'Frame'");
                ++i; // "Time:"
            } else if (k1 == "frametime") {
                ++i; // "FrameTime:"
            } else {
                return fail("esperava 'Frame Time:'");
            }
        }
        anim.dt = nextFloat("Frame Time:");
        if (anim.dt <= 0.0f) anim.dt = 1.0f / 30.0f;

        anim.valuesPerFrame = static_cast<size_t>(valueIndex);
        size_t total = anim.valuesPerFrame * static_cast<size_t>(anim.frameCount);
        anim.values.reserve(total);
        for (size_t k = 0; k < total; ++k) {
            if (!have()) return fail("dados de movimento truncados");
            anim.values.push_back(nextFloat("MOTION values"));
        }
        return true;
    }

    bool parseNode(int parent, bool isRoot) {
        if (!have()) return fail("fim inesperado na hierarquia");
        std::string kw = normKey(peek());
        if (kw != "root" && kw != "joint") {
            return fail(std::string("esperava ROOT/JOINT, encontrei '") + peek() + "'");
        }
        ++i;
        if (!have()) return fail("nome do osso ausente");
        std::string name = next();
        std::string brace;
        if (!expect("{", brace)) return false;

        SkeletonBone bone;
        bone.name = name;
        bone.parent = parent;
        bone.offset = {0, 0, 0};
        int boneIndex = static_cast<int>(anim.skeleton.bones.size());
        anim.skeleton.bones.push_back(bone);
        anim.boneInfo.push_back(BvhBoneInfo{});

        while (true) {
            if (!have()) return fail("bloco do osso '" + name + "' não fechado");
            std::string t = peek();
            std::string key = normKey(t);
            if (key == "}") { ++i; break; }
            if (key == "offset") {
                ++i;
                bone.offset = {nextFloat("OFFSET"), nextFloat("OFFSET"), nextFloat("OFFSET")};
                anim.skeleton.bones[boneIndex].offset = bone.offset;
                continue;
            }
            if (key == "channels") {
                ++i;
                int n = nextInt("CHANNELS");
                for (int c = 0; c < n; ++c) {
                    if (!have()) return fail("canais truncados");
                    std::string chName = next();
                    // Forma: <Axis><Type>, ex.: Xposition / Zrotation
                    char axis = static_cast<char>(std::tolower(static_cast<unsigned char>(chName[0])));
                    std::string type;
                    for (size_t k = 1; k < chName.size(); ++k) type += static_cast<char>(std::tolower(static_cast<unsigned char>(chName[k])));
                    bool isPos = (type == "position");
                    BvhChannel ch;
                    ch.valueIndex = valueIndex++;
                    ch.axis = axis;
                    ch.isPosition = isPos;
                    anim.boneInfo[boneIndex].channels.push_back(ch);
                }
                continue;
            }
            if (key == "joint" || key == "root") {
                if (!parseNode(boneIndex, false)) return false;
                continue;
            }
            if (key == "end") {
                // "End Site { OFFSET x y z }"
                ++i;
                std::string site;
                if (normKey(peek()) == "site") { ++i; }
                if (!expect("{", site)) return false;
                if (normKey(peek()) != "offset") return fail("End Site sem OFFSET");
                ++i;
                Vec3 off{nextFloat("End OFFSET"), nextFloat("End OFFSET"), nextFloat("End OFFSET")};
                (void)off;
                std::string close;
                if (!expect("}", close)) return false;
                // End Site não recebe canais; ignorado no esqueleto canônico.
                continue;
            }
            ++i; // token desconhecido: pular defensivamente
        }
        (void)isRoot;
        return true;
    }
};

bool loadBvhFromString(const std::string& content, BvhAnim& out, std::string& error) {
    out = BvhAnim{};
    std::vector<std::string> toks = tokenize(content);
    if (toks.empty()) { error = "arquivo vazio"; return false; }
    BvhParser parser(std::move(toks), out, error);
    return parser.parse();
}

bool loadBvhFile(const std::string& path, BvhAnim& out, std::string& error) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { error = "não foi possível abrir: " + path; return false; }
    std::ostringstream ss;
    ss << f.rdbuf();
    return loadBvhFromString(ss.str(), out, error);
}

Quat BvhAnim::boneRotation(int bone, int frame) const {
    if (bone < 0 || bone >= static_cast<int>(boneInfo.size())) return Quat::identity();
    if (frame < 0 || frame >= frameCount) return Quat::identity();
    const float* base = values.data() + static_cast<size_t>(frame) * valuesPerFrame;
    Quat q = Quat::identity();
    // Canais de rotação na ordem declarada; primeiro declarado é aplicado primeiro.
    for (const auto& ch : boneInfo[bone].channels) {
        if (ch.isPosition) continue;
        float angle = degToRad(base[ch.valueIndex]);
        q = quatFromAxisAngle(ch.axis, angle) * q;
    }
    return q.normalized();
}

Vec3 BvhAnim::bonePosition(int bone, int frame) const {
    if (bone < 0 || bone >= static_cast<int>(boneInfo.size())) return {0, 0, 0};
    if (frame < 0 || frame >= frameCount) return skeleton.bones[bone].offset;
    const float* base = values.data() + static_cast<size_t>(frame) * valuesPerFrame;
    Vec3 p = skeleton.bones[bone].offset;
    bool any = false;
    Vec3 chans{0, 0, 0};
    for (const auto& ch : boneInfo[bone].channels) {
        if (!ch.isPosition) continue;
        any = true;
        float v = base[ch.valueIndex];
        if (ch.axis == 'x') chans.x = v;
        else if (ch.axis == 'y') chans.y = v;
        else if (ch.axis == 'z') chans.z = v;
    }
    // Posição absoluta do canal (BVH guarda posição local absoluta da raiz).
    return any ? chans : p;
}

} // namespace pm
