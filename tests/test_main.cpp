// PESMotion - testes de regressão (ctest). Sem framework: asserts + relatório.
#include "math3d.h"
#include "bvh.h"
#include "pmca.h"
#include "pesbin.h"
#include "retarget.h"
#include "skeleton.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <string>

using namespace pm;

static int g_failures = 0;
#define CHECK(name, cond) do { \
    if (cond) { std::printf("PASS %s\n", name); } \
    else { std::printf("FAIL %s (linha %d)\n", name, __LINE__); ++g_failures; } \
} while (0)

const char* kBvh =
    "HIERARCHY\nROOT Hips\n{\n\tOFFSET 0 0 0\n"
    "\tCHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation\n"
    "\tJOINT Spine\n\t{\n\t\tOFFSET 0 0.1 0\n"
    "\t\tCHANNELS 3 Zrotation Xrotation Yrotation\n"
    "\t\tEnd Site\n\t\t{\n\t\t\tOFFSET 0 0.1 0\n\t\t}\n\t}\n}\n"
    "MOTION\nFrames: 3\nFrame Time: 0.033333\n"
    "0 0.9 0 0 0 0 0 0 0\n"
    "0 0.9 0 0 1 0 0 1 0\n"
    "0 0.91 0 0 2 0 0 2 0\n";

int main() {
    // Matemática básica
    {
        Quat q = quatFromAxisAngle('z', kPi / 2.0f);
        Vec3 v = quatRotate(q, Vec3{1, 0, 0});
        CHECK("rotate-90-z", std::fabs(v.y - 1.0f) < 1e-5f && std::fabs(v.x) < 1e-5f);
        Quat n = q.normalized();
        CHECK("quat-normalized", std::fabs(n.length() - 1.0f) < 1e-6f);
    }
    // Parser
    {
        BvhAnim anim; std::string err;
        bool ok = loadBvhFromString(kBvh, anim, err);
        CHECK("bvh-parse", ok);
        if (ok) {
            CHECK("bvh-3-frames", anim.frameCount == 3);
            CHECK("bvh-2-bones", anim.skeleton.boneCount() == 2);
            CHECK("bvh-9-channels", anim.valuesPerFrame == 9);
        }
    }
    // PMCA roundtrip
    {
        BvhAnim anim; std::string err;
        if (loadBvhFromString(kBvh, anim, err)) {
            RetargetOptions opt; opt.rootTranslationScale = 1.0f;
            std::string rerr;
            AnimClip clip = convertBvhToClip(anim, "test", opt, rerr);
            CHECK("retarget-clip", clip.frameCount == 3 && clip.boneCount == kCanonicalBoneOrderSize);

            std::vector<uint8_t> bytes; std::string werr;
            CHECK("pmca-bytes", writePmcaBytes(clip, bytes, werr));
            AnimClip back; std::string r2;
            CHECK("pmca-read", readPmcaBytes(bytes, back, r2));
            if (back.frameCount == clip.frameCount) {
                float maxErr = 0.0f;
                for (size_t i = 0; i < clip.rotations.size(); ++i)
                    maxErr = std::fmax(maxErr, quatAngleBetween(clip.rotations[i], back.rotations[i]));
                CHECK("pmca-error-small", maxErr < 1e-3f);
            }
        } else {
            CHECK("retarget-clip", false);
        }
    }
    // Redução
    {
        AnimClip clip; clip.name = "s"; clip.boneCount = kCanonicalBoneOrderSize;
        clip.frameCount = 30; clip.dt = 1.0f / 30.0f;
        clip.rotations.assign(static_cast<size_t>(30) * clip.boneCount, Quat::identity());
        clip.rootMotion.assign(30, Vec3{0, 0.9f, 0});
        int removed = reduceAnimClip(clip, degToRad(0.5f), 0.01f);
        CHECK("reduce-static", clip.frameCount == 2 && removed == 28);
    }
    // Perfis e PMB
    {
        CHECK("9-profiles", pesProfiles().size() == 9);
        const PesProfile* p = findPesProfile("pes2021");
        CHECK("pes2021", p != nullptr);
        AnimClip clip; clip.name = "d"; clip.boneCount = kCanonicalBoneOrderSize;
        clip.frameCount = 2; clip.dt = 1.0f / 60.0f;
        clip.rotations.assign(static_cast<size_t>(2) * clip.boneCount, Quat::identity());
        clip.rootMotion.assign(2, Vec3{0, 0, 0});
        std::vector<AnimClip> clips{clip};
        std::vector<uint8_t> bytes; std::string err;
        CHECK("pesbin-bytes", p && buildPesBinBytes(*p, clips, bytes, err));
        CHECK("pesbin-magic", bytes.size() > 4 && bytes[0] == 'P' && bytes[1] == 'M');
    }

    std::printf("\n%s (%d falha%s)\n", g_failures == 0 ? "TODOS OS TESTES PASSARAM" : "HOUVE FALHAS",
                g_failures, g_failures == 1 ? "" : "s");
    return g_failures == 0 ? 0 : 1;
}
