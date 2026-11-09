#pragma once
// PESMotion - bvh: importador de arquivos BVH (Biovision Hierarchy), formato
// aberto padrão de intercâmbio de captura. Fonte primária dos datasets abertos.

#include "math3d.h"
#include "skeleton.h"
#include <string>
#include <vector>

namespace pm {

struct BvhChannel {
    int valueIndex = -1; // índice na fita de valores por frame
    char axis = 'x';     // 'x' | 'y' | 'z'
    bool isPosition = false;
};

struct BvhBoneInfo {
    std::vector<BvhChannel> channels;
};

struct BvhAnim {
    Skeleton skeleton;
    std::vector<BvhBoneInfo> boneInfo;
    int frameCount = 0;
    float dt = 1.0f / 30.0f;
    std::vector<float> values;      // frameCount * valuesPerFrame
    size_t valuesPerFrame = 0;

    // Rotação local do osso `bone` no frame `frame` (identidade se não houver canais).
    Quat boneRotation(int bone, int frame) const;
    // Posição local do osso `bone` no frame `frame` (offset da hierarquia se
    // não houver canais de posição).
    Vec3 bonePosition(int bone, int frame) const;
};

bool loadBvhFromString(const std::string& content, BvhAnim& out, std::string& error);
bool loadBvhFile(const std::string& path, BvhAnim& out, std::string& error);

} // namespace pm
