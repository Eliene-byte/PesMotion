#pragma once
// PESMotion - math3d: vetores e quaternions mínimos e auditáveis.
// Sem dependências externas. Hamilton convention (x, y, z, w).

#include <cstdint>
#include <cmath>

namespace pm {

constexpr float kPi = 3.14159265358979323846f;
inline float degToRad(float d) { return d * (kPi / 180.0f); }

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vec3() = default;
    Vec3(float px, float py, float pz) : x(px), y(py), z(pz) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }

    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    float length() const { return std::sqrt(dot(*this)); }
    Vec3 normalized() const {
        float l = length();
        return (l > 1e-8f) ? (*this) * (1.0f / l) : Vec3{0.0f, 0.0f, 0.0f};
    }
};

struct Quat {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
    Quat() = default;
    Quat(float px, float py, float pz, float pw) : x(px), y(py), z(pz), w(pw) {}

    static Quat identity() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    // b * a  ==  aplica a seguido de b (convenção Hamilton).
    Quat operator*(const Quat& o) const {
        return {
            w * o.x + x * o.w + y * o.z - z * o.y,
            w * o.y - x * o.z + y * o.w + z * o.x,
            w * o.z + x * o.y - y * o.x + z * o.w,
            w * o.w - x * o.x - y * o.y - z * o.z,
        };
    }
    Quat conjugated() const { return {-x, -y, -z, w}; }
    Quat inverse() const {
        float n2 = x * x + y * y + z * z + w * w;
        if (n2 < 1e-12f) return identity();
        float inv = 1.0f / n2;
        return {-x * inv, -y * inv, -z * inv, w * inv};
    }
    Quat normalized() const {
        float l = std::sqrt(x * x + y * y + z * z + w * w);
        return (l > 1e-8f) ? Quat{x / l, y / l, z / l, w / l} : identity();
    }
    float length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
    float dot(const Quat& o) const { return x * o.x + y * o.y + z * o.z + w * o.w; }
};

// Eixo único: 'x', 'y' ou 'z'.
Quat quatFromAxisAngle(char axis, float radians);

// Constrói um quaternion a partir de 3 ângulos de Euler (radianos).
// A ordem listada é a ordem de aplicação (primeiro eixo listado é o primeiro
// giro aplicado); convenção Hamilton com rotações ativas.
Quat quatFromEulerOrder(const char order[3], float a0, float a1, float a2);

// SLERP com caminho mais curto (corrige sinal se necessário). t em [0, 1].
Quat quatSlerp(const Quat& a, const Quat& b, float t);

// Rotaciona um vetor por um quaternion unitário.
Vec3 quatRotate(const Quat& q, const Vec3& v);

// Diferença angular entre duas orientações, em radianos (sempre >= 0).
float quatAngleBetween(const Quat& a, const Quat& b);

} // namespace pm
