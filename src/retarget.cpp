#include "retarget.h"
#include "skeleton.h"
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace pm {

static std::string normalizeBoneName(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc)) out += static_cast<char>(std::tolower(uc));
    }
    return out;
}

// Tabela de aliases comuns (CMU/SFU/Mixamo/Biped) → osso canônico PSM-22.
static const std::unordered_map<std::string, int>& aliasMap() {
    static const std::unordered_map<std::string, int> kMap = {
        {"root", 0}, {"hips", 1}, {"hip", 1}, {"pelvis", 1}, {"crotch", 1},
        {"spine", 2}, {"spine1", 3}, {"spine2", 3}, {"chest", 3}, {"thorax", 3}, {"upperchest", 3},
        {"neck", 4}, {"head", 5},
        {"leftshoulder", 6}, {"leftclavicle", 6}, {"clavicleleft", 6},
        {"leftarm", 7}, {"leftupperarm", 7}, {"leftuarm", 7},
        {"leftforearm", 8}, {"leftlowerarm", 8}, {"leftloarm", 8},
        {"lefthand", 9},
        {"rightshoulder", 10}, {"rightclavicle", 10}, {"clavicleright", 10},
        {"rightarm", 11}, {"rightupperarm", 11}, {"rightuarm", 11},
        {"rightforearm", 12}, {"rightlowerarm", 12}, {"rightloarm", 12},
        {"righthand", 13},
        {"leftupleg", 14}, {"leftthigh", 14}, {"leftupperleg", 14},
        {"leftleg", 15}, {"leftcalf", 15}, {"leftshin", 15}, {"leftlowerleg", 15},
        {"leftfoot", 16}, {"lefttoebase", 17}, {"lefttoe", 17}, {"leftball", 17},
        {"rightupleg", 18}, {"rightthigh", 18}, {"rightupperleg", 18},
        {"rightleg", 19}, {"rightcalf", 19}, {"rightshin", 19}, {"rightlowerleg", 19},
        {"rightfoot", 20}, {"righttoebase", 21}, {"righttoe", 21}, {"rightball", 21},
    };
    return kMap;
}

AnimClip convertBvhToClip(const BvhAnim& src, const std::string& clipName,
                          const RetargetOptions& opt, std::string& error) {
    AnimClip clip;
    clip.name = clipName;
    clip.boneCount = kCanonicalBoneOrderSize;
    clip.dt = src.dt;
    clip.frameCount = src.frameCount;
    if (src.frameCount <= 0) { error = "clip de origem sem frames"; return {}; }

    // Mapeia ossos de origem → índice canônico.
    std::vector<int> srcToCanon(src.skeleton.bones.size(), -1);
    for (int i = 0; i < static_cast<int>(src.skeleton.bones.size()); ++i) {
        std::string norm = normalizeBoneName(src.skeleton.bones[i].name);
        auto it = aliasMap().find(norm);
        if (it != aliasMap().end()) srcToCanon[i] = it->second;
    }

    const int B = clip.boneCount;
    clip.rotations.assign(static_cast<size_t>(clip.frameCount) * B, Quat::identity());
    clip.rootMotion.assign(clip.frameCount, Vec3{0, 0, 0});

    for (int f = 0; f < clip.frameCount; ++f) {
        // Ossos de origem primeiro (a ordem não importa: cada bone escreve o seu slot).
        for (int s = 0; s < static_cast<int>(src.skeleton.bones.size()); ++s) {
            int c = srcToCanon[s];
            if (c < 0) continue;
            clip.rotations[static_cast<size_t>(f) * B + c] = src.boneRotation(s, f);
        }
        Vec3 root = src.bonePosition(0, f);
        root = root * opt.rootTranslationScale;
        if (opt.zeroRootMotion) root = Vec3{0, 0, 0};
        // Normaliza: chão em y=0 na pose neutra é responsabilidade do palco, não do clip.
        clip.rootMotion[f] = root;
    }
    (void)error;
    return clip;
}

int reduceAnimClip(AnimClip& clip, float minAngleRad, float minRootDelta) {
    // Implementação simples e determinística: greedy sobre o clip atual.
    if (clip.frameCount <= 2) return 0;
    const int B = clip.boneCount;

    AnimClip reduced;
    reduced.name = clip.name;
    reduced.boneCount = B;
    reduced.dt = clip.dt;

    auto pushFrame = [&](int f) {
        for (int b = 0; b < B; ++b) reduced.rotations.push_back(clip.rotationAt(f, b));
        reduced.rootMotion.push_back(clip.rootMotion[f]);
        reduced.frameCount++;
    };

    pushFrame(0);
    int lastKept = 0;
    for (int f = 1; f < clip.frameCount - 1; ++f) {
        bool significant = false;
        for (int b = 0; b < B; ++b) {
            if (quatAngleBetween(clip.rotationAt(lastKept, b), clip.rotationAt(f, b)) > minAngleRad) {
                significant = true; break;
            }
        }
        if (!significant) {
            Vec3 d = clip.rootMotion[f] - clip.rootMotion[lastKept];
            if (d.length() > minRootDelta) significant = true;
        }
        if (significant) { pushFrame(f); lastKept = f; }
    }
    pushFrame(clip.frameCount - 1);

    int removed = clip.frameCount - reduced.frameCount;
    clip.rotations.swap(reduced.rotations);
    clip.rootMotion.swap(reduced.rootMotion);
    clip.frameCount = reduced.frameCount;
    return removed;
}

} // namespace pm
