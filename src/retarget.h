#pragma once
// PESMotion - retarget: converte captura BVH para o esqueleto canônico PSM-22
// e aplica redução de keyframes (animações ultra-leves).

#include "bvh.h"
#include "math3d.h"
#include <string>
#include <vector>

namespace pm {

struct AnimClip {
    std::string name;
    int boneCount = 0;   // kCanonicalBoneOrderSize
    int frameCount = 0;
    float dt = 1.0f / 60.0f;
    std::vector<Quat> rotations;  // frameCount * boneCount
    std::vector<Vec3> rootMotion; // frameCount — posição absoluta da raiz

    Quat rotationAt(int frame, int bone) const {
        return rotations[static_cast<size_t>(frame) * boneCount + bone];
    }
};

struct RetargetOptions {
    float rootTranslationScale = 1.0f; // escala da translação da raiz
    bool zeroRootMotion = false;       // zera deslocamento de raiz (in-place)
};

// Converte um BVH para o esqueleto canônico. Retorna clip vazio + erro em falha.
AnimClip convertBvhToClip(const BvhAnim& src, const std::string& clipName,
                          const RetargetOptions& opt, std::string& error);

// Remove keyframes redundantes: mantém frames onde alguma orientação de osso
// gira mais que `minAngleRad` ou a raiz desloca mais que `minRootDelta` (m).
// Sempre preserva primeiro e último frame. Retorna nº de frames removidos.
int reduceAnimClip(AnimClip& clip, float minAngleRad, float minRootDelta);

} // namespace pm
