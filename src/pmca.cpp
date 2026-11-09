#include "pmca.h"
#include <fstream>
#include <cstring>
#include <cmath>

namespace pm {

namespace {
constexpr uint8_t kMagic[8] = {'P', 'M', 'C', 'A', '0', '0', '0', '1'};

void putU16(std::vector<uint8_t>& o, uint16_t v) { o.push_back(v & 0xFF); o.push_back((v >> 8) & 0xFF); }
void putU32(std::vector<uint8_t>& o, uint32_t v) {
    o.push_back(v & 0xFF); o.push_back((v >> 8) & 0xFF);
    o.push_back((v >> 16) & 0xFF); o.push_back((v >> 24) & 0xFF);
}
void putI16(std::vector<uint8_t>& o, int16_t v) { putU16(o, static_cast<uint16_t>(v)); }
void putF32(std::vector<uint8_t>& o, float f) {
    uint32_t u; std::memcpy(&u, &f, 4); putU32(o, u);
}

struct Reader {
    const uint8_t* p; size_t n, off = 0;
    bool u8(uint8_t& v) { if (off + 1 > n) return false; v = p[off++]; return true; }
    bool u16(uint16_t& v) {
        if (off + 2 > n) return false;
        v = static_cast<uint16_t>(p[off] | (p[off + 1] << 8)); off += 2; return true;
    }
    bool u32(uint32_t& v) {
        if (off + 4 > n) return false;
        v = static_cast<uint32_t>(p[off]) | (static_cast<uint32_t>(p[off + 1]) << 8) |
            (static_cast<uint32_t>(p[off + 2]) << 16) | (static_cast<uint32_t>(p[off + 3]) << 24);
        off += 4; return true;
    }
    bool i16(int16_t& v) { uint16_t u; if (!u16(u)) return false; v = static_cast<int16_t>(u); return true; }
    bool f32(float& f) { uint32_t u; if (!u32(u)) return false; std::memcpy(&f, &u, 4); return true; }
    bool bytes(std::string& s, size_t count) {
        if (off + count > n) return false;
        s.assign(reinterpret_cast<const char*>(p + off), count); off += count; return true;
    }
};

int16_t quantizeQuatComponent(float c) {
    float clamped = c < -1.0f ? -1.0f : (c > 1.0f ? 1.0f : c);
    return static_cast<int16_t>(std::lround(clamped * 32767.0f));
}
float dequantizeQuatComponent(int16_t v) { return static_cast<float>(v) / 32767.0f; }
} // namespace

bool writePmcaBytes(const AnimClip& clip, std::vector<uint8_t>& out, std::string& error) {
    if (clip.boneCount <= 0 || clip.frameCount <= 0) { error = "clip vazio"; return false; }
    if (static_cast<int>(clip.rotations.size()) != clip.frameCount * clip.boneCount) {
        error = "rotations em tamanho incompatível"; return false;
    }

    out.clear();
    out.reserve(64 + static_cast<size_t>(clip.frameCount) * clip.boneCount * 8 +
                static_cast<size_t>(clip.frameCount) * 12 + clip.name.size());

    out.insert(out.end(), kMagic, kMagic + 8);
    putU16(out, 1);                          // container version
    putU16(out, static_cast<uint16_t>(clip.boneCount));
    putU32(out, static_cast<uint32_t>(clip.frameCount));
    putF32(out, clip.dt);
    uint8_t flags = clip.rootMotion.empty() ? 0 : PMCA_FLAGS_ROOT_MOTION;
    out.push_back(flags);
    putU16(out, static_cast<uint16_t>(clip.name.size()));
    out.insert(out.end(), clip.name.begin(), clip.name.end());

    // Continuidade hemisférica: força dot(q_prev, q) >= 0 por osso.
    std::vector<Quat> prev(clip.boneCount, Quat::identity());
    for (int f = 0; f < clip.frameCount; ++f) {
        for (int b = 0; b < clip.boneCount; ++b) {
            Quat q = clip.rotationAt(f, b).normalized();
            if (f > 0 && q.dot(prev[b]) < 0.0f) q = Quat{-q.x, -q.y, -q.z, -q.w};
            prev[b] = q;
            putI16(out, quantizeQuatComponent(q.x));
            putI16(out, quantizeQuatComponent(q.y));
            putI16(out, quantizeQuatComponent(q.z));
            putI16(out, quantizeQuatComponent(q.w));
        }
        if (flags & PMCA_FLAGS_ROOT_MOTION) {
            const Vec3& p = clip.rootMotion[f];
            putF32(out, p.x); putF32(out, p.y); putF32(out, p.z);
        }
    }
    (void)error;
    return true;
}

bool readPmcaBytes(const std::vector<uint8_t>& in, AnimClip& out, std::string& error) {
    Reader r{in.data(), in.size(), 0};
    uint8_t magic[8];
    for (int i = 0; i < 8; ++i) { if (!r.u8(magic[i])) { error = "arquivo truncado (magic)"; return false; } }
    if (std::memcmp(magic, kMagic, 8) != 0) { error = "magic PMCA inválido"; return false; }

    uint16_t version = 0, boneCount = 0, nameLen = 0;
    uint32_t frameCount = 0;
    float dt = 0.0f;
    uint8_t flags = 0;
    if (!r.u16(version) || version != 1) { error = "versão PMCA não suportada"; return false; }
    if (!r.u16(boneCount) || boneCount == 0 || boneCount > 512) { error = "boneCount inválido"; return false; }
    if (!r.u32(frameCount) || frameCount == 0 || frameCount > 10'000'000u) { error = "frameCount inválido"; return false; }
    if (!r.f32(dt) || dt <= 0.0f) { error = "dt inválido"; return false; }
    if (!r.u8(flags)) { error = "truncado (flags)"; return false; }
    if (!r.u16(nameLen)) { error = "truncado (nome)"; return false; }

    AnimClip clip;
    clip.boneCount = boneCount;
    clip.frameCount = static_cast<int>(frameCount);
    clip.dt = dt;
    if (!r.bytes(clip.name, nameLen)) { error = "nome truncado"; return false; }

    const size_t needed = static_cast<size_t>(frameCount) * boneCount * 8 +
                          ((flags & PMCA_FLAGS_ROOT_MOTION) ? static_cast<size_t>(frameCount) * 12 : 0);
    if (r.off + needed > r.n) { error = "dados de frames truncados"; return false; }

    clip.rotations.resize(static_cast<size_t>(frameCount) * boneCount);
    clip.rootMotion.assign(frameCount, Vec3{0, 0, 0});

    for (uint32_t f = 0; f < frameCount; ++f) {
        for (uint16_t b = 0; b < boneCount; ++b) {
            int16_t x = 0, y = 0, z = 0, w = 0;
            r.i16(x); r.i16(y); r.i16(z); r.i16(w);
            clip.rotations[static_cast<size_t>(f) * boneCount + b] =
                Quat{dequantizeQuatComponent(x), dequantizeQuatComponent(y),
                     dequantizeQuatComponent(z), dequantizeQuatComponent(w)}.normalized();
        }
        if (flags & PMCA_FLAGS_ROOT_MOTION) {
            float x, y, z; r.f32(x); r.f32(y); r.f32(z);
            clip.rootMotion[f] = Vec3{x, y, z};
        }
    }
    out = std::move(clip);
    return true;
}

bool writePmcaFile(const AnimClip& clip, const std::string& path, std::string& error) {
    std::vector<uint8_t> bytes;
    if (!writePmcaBytes(clip, bytes, error)) return false;
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) { error = "não foi possível escrever: " + path; return false; }
    f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(f);
}

bool readPmcaFile(const std::string& path, AnimClip& out, std::string& error) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { error = "não foi possível abrir: " + path; return false; }
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return readPmcaBytes(bytes, out, error);
}

} // namespace pm
