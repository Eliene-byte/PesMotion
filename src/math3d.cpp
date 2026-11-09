#include "math3d.h"
#include <algorithm>

namespace pm {

Quat quatFromAxisAngle(char axis, float radians) {
    float half = radians * 0.5f;
    float s = std::sin(half);
    float c = std::cos(half);
    switch (axis) {
        case 'x': case 'X': return {s, 0.0f, 0.0f, c};
        case 'y': case 'Y': return {0.0f, s, 0.0f, c};
        case 'z': case 'Z': return {0.0f, 0.0f, s, c};
        default: return Quat::identity();
    }
}

Quat quatFromEulerOrder(const char order[3], float a0, float a1, float a2) {
    // Primeiro eixo listado é aplicado primeiro (semântica BVH);
    // q_total = q_axis2 * q_axis1 * q_axis0.
    Quat r = quatFromAxisAngle(order[0], a0);
    r = quatFromAxisAngle(order[1], a1) * r;
    r = quatFromAxisAngle(order[2], a2) * r;
    return r.normalized();
}

Quat quatSlerp(const Quat& aIn, const Quat& bIn, float t) {
    Quat a = aIn.normalized();
    Quat b = bIn.normalized();
    float cosTheta = a.dot(b);
    if (cosTheta < 0.0f) {
        b = Quat{-b.x, -b.y, -b.z, -b.w};
        cosTheta = -cosTheta;
    }
    if (cosTheta > 0.9995f) {
        // Perto demais: interpolação linear + normalização.
        Quat r{
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t,
        };
        return r.normalized();
    }
    float theta = std::acos(std::clamp(cosTheta, -1.0f, 1.0f));
    float sinTheta = std::sin(theta);
    float wa = std::sin((1.0f - t) * theta) / sinTheta;
    float wb = std::sin(t * theta) / sinTheta;
    return {
        a.x * wa + b.x * wb,
        a.y * wa + b.y * wb,
        a.z * wa + b.z * wb,
        a.w * wa + b.w * wb,
    };
}

Vec3 quatRotate(const Quat& q, const Vec3& v) {
    Quat p{v.x, v.y, v.z, 0.0f};
    Quat r = q * p * q.conjugated();
    return {r.x, r.y, r.z};
}

float quatAngleBetween(const Quat& a, const Quat& b) {
    Quat d = b * a.conjugated();
    d = d.normalized();
    float c = std::clamp(std::fabs(d.w), 0.0f, 1.0f);
    return 2.0f * std::acos(c);
}

} // namespace pm
