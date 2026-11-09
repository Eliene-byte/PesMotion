#include "skeleton.h"
#include <stdexcept>

namespace pm {

int Skeleton::findBone(const std::string& name) const {
    for (int i = 0; i < static_cast<int>(bones.size()); ++i) {
        if (bones[i].name == name) return i;
    }
    return -1;
}

const char* const kCanonicalBoneOrder[] = {
    "root",      "hips",     "spine", "chest", "neck", "head",
    "clavicle_l","uarm_l",   "loarm_l","hand_l",
    "clavicle_r","uarm_r",   "loarm_r","hand_r",
    "upleg_l",   "loleg_l",  "foot_l", "toe_l",
    "upleg_r",   "loleg_r",  "foot_r", "toe_r",
};
const int kCanonicalBoneOrderSize =
    static_cast<int>(sizeof(kCanonicalBoneOrder) / sizeof(kCanonicalBoneOrder[0]));

// Esqueleto canônico: proporções medianas para atleta adulto (~1.83 m),
// derivadas de tabelas antropométricas padrão (Winter; Drillis & Contini).
// Ajustável por versão de jogo no retargeter (escala uniforme e por eixo).
Skeleton makeCanonicalSkeleton(float heightMeters) {
    const float h = heightMeters;
    Skeleton s;
    auto add = [&](const char* name, int parent, float x, float y, float z) {
        SkeletonBone b;
        b.name = name;
        b.parent = parent;
        b.offset = {x, y, z};
        s.bones.push_back(b);
        return static_cast<int>(s.bones.size()) - 1;
    };

    const int root = add("root", -1, 0.0f, 0.0f, 0.0f);                 // pélvis (chão)
    const int hips = add("hips", root, 0.0f, 0.530f * h, 0.0f);         // centro de massa
    const int spine = add("spine", hips, 0.0f, 0.085f * h, 0.0f);
    const int chest = add("chest", spine, 0.0f, 0.115f * h, 0.0f);
    const int neck = add("neck", chest, 0.0f, 0.135f * h, 0.0f);
    add("head", neck, 0.0f, 0.065f * h, 0.0f);                          // head

    const int clL = add("clavicle_l", chest, 0.075f * h, 0.115f * h, 0.0f);
    const int uaL = add("uarm_l", clL, 0.160f * h, 0.0f, 0.0f);
    const int laL = add("loarm_l", uaL, 0.270f * h, 0.0f, 0.0f);
    add("hand_l", laL, 0.250f * h, 0.0f, 0.0f);

    const int clR = add("clavicle_r", chest, -0.075f * h, 0.115f * h, 0.0f);
    const int uaR = add("uarm_r", clR, -0.160f * h, 0.0f, 0.0f);
    const int laR = add("loarm_r", uaR, -0.270f * h, 0.0f, 0.0f);
    add("hand_r", laR, -0.250f * h, 0.0f, 0.0f);

    const int ulL = add("upleg_l", hips, 0.095f * h, -0.045f * h, 0.0f);
    const int llL = add("loleg_l", ulL, 0.020f * h, -0.245f * h, 0.0f);
    const int ftL = add("foot_l", llL, 0.0f, -0.240f * h, 0.0f);
    add("toe_l", ftL, 0.0f, -0.045f * h, 0.085f * h);

    const int ulR = add("upleg_r", hips, -0.095f * h, -0.045f * h, 0.0f);
    const int llR = add("loleg_r", ulR, -0.020f * h, -0.245f * h, 0.0f);
    const int ftR = add("foot_r", llR, 0.0f, -0.240f * h, 0.0f);
    add("toe_r", ftR, 0.0f, -0.045f * h, 0.085f * h);

    (void)root; (void)hips; (void)spine; (void)chest; (void)neck;
    (void)clL; (void)uaL; (void)laL; (void)clR; (void)uaR; (void)laR;
    (void)ulL; (void)llL; (void)ftL; (void)ulR; (void)llR; (void)ftR;
    return s;
}

static PesProfile makeProfile(const char* id, const char* title, int year,
                              uint16_t formatVersion, uint32_t keyframeHeaderBytes) {
    PesProfile p;
    p.id = id;
    p.gameTitle = title;
    p.year = year;
    p.magic = 0x31424D50u; // 'PMB1' little-endian
    p.formatVersion = formatVersion;
    p.keyframeHeaderBytes = keyframeHeaderBytes;
    p.experimental = true;
    // IDs de ossos: PLACEHOLDER sequencial canônico. Ver docs/FORMATS.md.
    for (int i = 0; i < kCanonicalBoneOrderSize; ++i) p.pesBoneIds.push_back(i);
    return p;
}

const std::vector<PesProfile>& pesProfiles() {
    static const std::vector<PesProfile> kProfiles = {
        makeProfile("pes2013", "Pro Evolution Soccer 2013", 2013, 1, 64),
        makeProfile("pes2014", "Pro Evolution Soccer 2014", 2014, 2, 64),
        makeProfile("pes2015", "Pro Evolution Soccer 2015", 2015, 3, 64),
        makeProfile("pes2016", "Pro Evolution Soccer 2016", 2016, 4, 64),
        makeProfile("pes2017", "Pro Evolution Soccer 2017", 2017, 5, 56),
        makeProfile("pes2018", "Pro Evolution Soccer 2018", 2018, 6, 56),
        makeProfile("pes2019", "Pro Evolution Soccer 2019", 2019, 7, 56),
        makeProfile("pes2020", "eFootball PES 2020", 2020, 8, 48),
        makeProfile("pes2021", "eFootball PES 2021", 2021, 9, 48),
    };
    return kProfiles;
}

const PesProfile* findPesProfile(const std::string& id) {
    for (const auto& p : pesProfiles()) {
        if (id == p.id) return &p;
    }
    return nullptr;
}

} // namespace pm
