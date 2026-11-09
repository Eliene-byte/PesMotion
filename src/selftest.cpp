#include "selftest.h"
#include "math3d.h"
#include "bvh.h"
#include "pmca.h"
#include "pesbin.h"
#include "retarget.h"
#include "skeleton.h"
#include <cstdio>
#include <cmath>
#include <vector>

namespace pm {

namespace {
int g_failures = 0;

void check(const char* name, bool ok) {
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++g_failures;
}

bool near(float a, float b, float eps) { return std::fabs(a - b) <= eps; }

const char* kFixtureBvh =
    "HIERARCHY\n"
    "ROOT Hips\n"
    "{\n"
    "\tOFFSET 0.0 0.0 0.0\n"
    "\tCHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation\n"
    "\tJOINT LeftUpLeg\n"
    "\t{\n"
    "\t\tOFFSET 0.1 -0.05 0.0\n"
    "\t\tCHANNELS 3 Zrotation Xrotation Yrotation\n"
    "\t\tEnd Site\n"
    "\t\t{\n"
    "\t\t\tOFFSET 0.0 -0.25 0.0\n"
    "\t\t}\n"
    "\t}\n"
    "}\n"
    "MOTION\n"
    "Frames: 2\n"
    "Frame Time: 0.033333\n"
    "0 0.9 0 0 0 0 10 5 0\n"
    "0.01 0.9 0 0 2 0 20 8 0\n";
} // namespace

int runSelfTests() {
    g_failures = 0;

    // 1. Quat identidade rotaciona vetor sem mudar.
    {
        Vec3 v{1, 2, 3};
        Vec3 r = quatRotate(Quat::identity(), v);
        check("quat-identity-rotates-identity", near(r.x, v.x, 1e-6f) && near(r.y, v.y, 1e-6f) && near(r.z, v.z, 1e-6f));
    }
    // 2. Euler zero → identidade.
    {
        Quat q = quatFromEulerOrder("ZXY", 0.0f, 0.0f, 0.0f);
        check("euler-zero-is-identity", std::fabs(q.w - 1.0f) < 1e-6f);
    }
    // 3. SLERP no ponto médio ≈ meio caminho angular.
    {
        Quat a = Quat::identity();
        Quat b = quatFromAxisAngle('y', degToRad(90.0f));
        Quat mid = quatSlerp(a, b, 0.5f);
        float ang = quatAngleBetween(a, mid);
        check("slerp-half-angle", near(ang, degToRad(45.0f), 1e-3f));
    }
    // 4. Parser BVH embutido.
    {
        BvhAnim anim; std::string err;
        bool ok = loadBvhFromString(kFixtureBvh, anim, err);
        check("bvh-parse-ok", ok);
        if (ok) {
            check("bvh-frame-count", anim.frameCount == 2);
            check("bvh-values-per-frame", anim.valuesPerFrame == 9);
            check("bvh-bone-count", anim.skeleton.boneCount() == 2);
            Quat q = anim.boneRotation(0, 0);
            check("bvh-identity-rotation-frame0", std::fabs(q.w - 1.0f) < 1e-5f);
        } else {
            std::printf("   motivo: %s\n", err.c_str());
            check("bvh-frame-count", false);
            check("bvh-values-per-frame", false);
            check("bvh-bone-count", false);
            check("bvh-identity-rotation-frame0", false);
        }
    }
    // 5. Roundtrip PMCA (quantização i16: erro < 1e-3).
    {
        BvhAnim anim; std::string err;
        if (loadBvhFromString(kFixtureBvh, anim, err)) {
            RetargetOptions opt;
            std::string rerr;
            AnimClip clip = convertBvhToClip(anim, "selftest", opt, rerr);
            std::vector<uint8_t> bytes; std::string werr;
            bool wrote = writePmcaBytes(clip, bytes, werr);
            check("pmca-write", wrote);
            AnimClip back; std::string rerr2;
            bool read = wrote && readPmcaBytes(bytes, back, rerr2);
            check("pmca-read", read);
            if (read) {
                float maxErr = 0.0f;
                for (size_t i = 0; i < clip.rotations.size(); ++i) {
                    maxErr = std::fmax(maxErr, quatAngleBetween(clip.rotations[i], back.rotations[i]));
                }
                check("pmca-roundtrip-error<1e-3", maxErr < 1e-3f);
                check("pmca-frame-count", back.frameCount == clip.frameCount);
            } else {
                check("pmca-roundtrip-error<1e-3", false);
                check("pmca-frame-count", false);
            }
        } else {
            check("pmca-write", false); check("pmca-read", false);
            check("pmca-roundtrip-error<1e-3", false); check("pmca-frame-count", false);
        }
    }
    // 6. Esqueleto canônico e perfis.
    {
        Skeleton sk = makeCanonicalSkeleton(1.83f);
        check("canonical-bone-count-22", sk.boneCount() == kCanonicalBoneOrderSize);
        check("canonical-order-size-22", kCanonicalBoneOrderSize == 22);
        check("profile-pes2021-exists", findPesProfile("pes2021") != nullptr);
        check("profile-pes2013-exists", findPesProfile("pes2013") != nullptr);
        check("profile-unknown-rejected", findPesProfile("pes2000") == nullptr);
        check("profiles-cover-2013-2021", pesProfiles().size() == 9);
    }
    // 7. Redução de keyframes em clip estático.
    {
        AnimClip clip;
        clip.name = "static";
        clip.boneCount = kCanonicalBoneOrderSize;
        clip.frameCount = 60;
        clip.dt = 1.0f / 60.0f;
        clip.rotations.assign(static_cast<size_t>(60) * clip.boneCount, Quat::identity());
        clip.rootMotion.assign(60, Vec3{0, 0, 0});
        int removed = reduceAnimClip(clip, degToRad(0.5f), 0.01f);
        check("reduce-static-clip-to-2-frames", clip.frameCount == 2 && removed == 58);
    }
    // 8. Montagem PMB (experimental) produz bytes com checksum.
    {
        AnimClip clip;
        clip.name = "dummy";
        clip.boneCount = kCanonicalBoneOrderSize;
        clip.frameCount = 4;
        clip.dt = 1.0f / 60.0f;
        clip.rotations.assign(static_cast<size_t>(4) * clip.boneCount,
                              quatFromAxisAngle('x', 0.1f));
        clip.rootMotion.assign(4, Vec3{0, 0, 0});
        std::vector<AnimClip> clips{clip};
        const PesProfile* p = findPesProfile("pes2021");
        std::vector<uint8_t> bytes; std::string err;
        bool ok = p && buildPesBinBytes(*p, clips, bytes, err);
        check("pesbin-build-ok", ok && bytes.size() > 64);
        if (ok) {
            check("pesbin-magic-pmb1", bytes[0] == 'P' && bytes[1] == 'M' && bytes[2] == 'B' && bytes[3] == '1');
        } else {
            check("pesbin-magic-pmb1", false);
            std::printf("   motivo: %s\n", err.c_str());
        }
    }

    std::printf("\nselftest: %s (%d falha%s)\n", g_failures == 0 ? "OK" : "FALHOU",
                g_failures, g_failures == 1 ? "" : "s");
    return g_failures;
}

} // namespace pm
