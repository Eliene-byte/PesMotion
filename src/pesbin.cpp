#include "pesbin.h"
#include <fstream>
#include <cstring>
#include <cmath>

namespace pm {

namespace {
void putU16(std::vector<uint8_t>& o, uint16_t v) { o.push_back(v & 0xFF); o.push_back((v >> 8) & 0xFF); }
void putU32(std::vector<uint8_t>& o, uint32_t v) {
    o.push_back(v & 0xFF); o.push_back((v >> 8) & 0xFF);
    o.push_back((v >> 16) & 0xFF); o.push_back((v >> 24) & 0xFF);
}
void putI16(std::vector<uint8_t>& o, int16_t v) { putU16(o, static_cast<uint16_t>(v)); }
void putF32(std::vector<uint8_t>& o, float f) { uint32_t u; std::memcpy(&u, &f, 4); putU32(o, u); }

int16_t qc(float c) {
    float clamped = c < -1.0f ? -1.0f : (c > 1.0f ? 1.0f : c);
    return static_cast<int16_t>(std::lround(clamped * 32767.0f));
}

uint32_t fnv1a(const uint8_t* data, size_t n) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; ++i) { h ^= data[i]; h *= 16777619u; }
    return h;
}
} // namespace

bool buildPesBinBytes(const PesProfile& profile, const std::vector<AnimClip>& clips,
                      std::vector<uint8_t>& out, std::string& error) {
    if (clips.empty()) { error = "nenhum clip fornecido"; return false; }
    const int boneCount = clips[0].boneCount;
    for (const auto& c : clips) {
        if (c.boneCount != boneCount) { error = "clips com boneCount divergente"; return false; }
        if (static_cast<int>(profile.pesBoneIds.size()) != boneCount) {
            error = "perfil com tabela de ossos incompatível"; return false;
        }
    }

    out.clear();
    putU32(out, profile.magic);                       // 'PMB1'
    putU16(out, profile.formatVersion);
    putU16(out, static_cast<uint16_t>(clips.size())); // clipCount
    putU16(out, static_cast<uint16_t>(boneCount));
    putU16(out, 0);                                   // reservado
    uint32_t totalFrames = 0;
    for (const auto& c : clips) totalFrames += static_cast<uint32_t>(c.frameCount);
    putU32(out, totalFrames);

    size_t payloadSizeOffset = out.size();
    putU32(out, 0); // payloadBytes (preenchido no final)

    std::vector<uint32_t> clipChecksums;
    clipChecksums.reserve(clips.size());

    for (const auto& clip : clips) {
        size_t clipStart = out.size();
        putU16(out, static_cast<uint16_t>(clip.name.size()));
        out.insert(out.end(), clip.name.begin(), clip.name.end());
        putU32(out, static_cast<uint32_t>(clip.frameCount));
        putF32(out, clip.dt);
        for (int b = 0; b < boneCount; ++b) putU16(out, static_cast<uint16_t>(profile.pesBoneIds[b]));
        for (int f = 0; f < clip.frameCount; ++f) {
            for (int b = 0; b < boneCount; ++b) {
                Quat q = clip.rotationAt(f, b).normalized();
                putI16(out, qc(q.x)); putI16(out, qc(q.y));
                putI16(out, qc(q.z)); putI16(out, qc(q.w));
            }
            const Vec3& p = clip.rootMotion[f];
            putF32(out, p.x); putF32(out, p.y); putF32(out, p.z);
        }
        clipChecksums.push_back(fnv1a(out.data() + clipStart, out.size() - clipStart));
    }

    for (uint32_t c : clipChecksums) putU32(out, c);

    uint32_t payloadBytes = static_cast<uint32_t>(out.size() - payloadSizeOffset - 4);
    out[payloadSizeOffset + 0] = payloadBytes & 0xFF;
    out[payloadSizeOffset + 1] = (payloadBytes >> 8) & 0xFF;
    out[payloadSizeOffset + 2] = (payloadBytes >> 16) & 0xFF;
    out[payloadSizeOffset + 3] = (payloadBytes >> 24) & 0xFF;

    uint32_t total = fnv1a(out.data(), out.size());
    putU32(out, total);
    (void)error;
    return true;
}

bool buildPesBinFile(const PesProfile& profile, const std::vector<AnimClip>& clips,
                     const std::string& outPath, std::string& error) {
    std::vector<uint8_t> bytes;
    if (!buildPesBinBytes(profile, clips, bytes, error)) return false;
    std::ofstream f(outPath, std::ios::binary | std::ios::trunc);
    if (!f) { error = "não foi possível escrever: " + outPath; return false; }
    f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(f);
}

} // namespace pm
