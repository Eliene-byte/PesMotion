#pragma once
// PESMotion - skeleton: esqueleto canônico PESMotion (PSM) e perfis por versão do jogo.
//
// O esqueleto canônico (PSM-22) é o alvo universal do retargeter: qualquer
// fonte de captura (CMU, SFU, captura própria) é mapeada para ele, e cada
// perfil de jogo (pes2013..pes2021) traduz para a tabela de ossos daquele jogo.
//
// ATENÇÃO (honestidade de engenharia): os IDs de ossos PES são dados de
// engenharia reversa. O valor 0 no perfil significa "sequencial canônico" e é
// um PLACEHOLDER até que a tabela real de cada jogo seja validada em campo.
// Veja docs/FORMATS.md para como validar e promover um perfil de experimental
// para estável.

#include "math3d.h"
#include <cstdint>
#include <string>
#include <vector>

namespace pm {

struct SkeletonBone {
    std::string name;   // nome canônico PESMotion
    int parent = -1;    // índice do osso pai (-1 = raiz)
    Vec3 offset;        // offset local a partir do pai, em metros (pose bind)
};

struct Skeleton {
    std::vector<SkeletonBone> bones;
    int boneCount() const { return static_cast<int>(bones.size()); }
    int findBone(const std::string& name) const;
};

// Esqueleto canônico com proporções parametrizadas pela altura total (m).
Skeleton makeCanonicalSkeleton(float heightMeters = 1.83f);

struct PesProfile {
    const char* id;          // "pes2013" ... "pes2021"
    const char* gameTitle;   // título oficial do jogo
    int year;
    uint32_t magic;          // mágica do contêiner experimental PESMotion-Bin
    uint16_t formatVersion;  // versão do layout experimental
    // PES bone id por osso canônico, na ordem de kCanonicalBoneOrder.
    // 0..N-1 sequencial = PLACEHOLDER experimental (RE pendente).
    std::vector<int> pesBoneIds; // tamanho == número de ossos canônicos
    uint32_t keyframeHeaderBytes;
    bool experimental;       // false somente após validação em campo
};

// Ordem canônica dos ossos no contêiner e na fita de ossos.
extern const char* const kCanonicalBoneOrder[];
extern const int kCanonicalBoneOrderSize;

const std::vector<PesProfile>& pesProfiles();  // PES 2013 .. PES 2021
const PesProfile* findPesProfile(const std::string& id);

} // namespace pm
