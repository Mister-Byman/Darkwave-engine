#pragma once
#include <cmath>
#include <cstring>
#include "math/Vec3.h"

// Column-major 4x4 (как в GLSL по умолчанию)
struct Mat4 {
    float m[16]{};

    static Mat4 identity() {
        Mat4 r;
        r.m[0] = 1; r.m[5] = 1; r.m[10] = 1; r.m[15] = 1;
        return r;
    }

    // r = a * b
    static Mat4 mul(const Mat4& a, const Mat4& b) {
        Mat4 r;
        for (int c = 0; c < 4; c++) {
            for (int r0 = 0; r0 < 4; r0++) {
                r.m[c * 4 + r0] =
                    a.m[0 * 4 + r0] * b.m[c * 4 + 0] +
                    a.m[1 * 4 + r0] * b.m[c * 4 + 1] +
                    a.m[2 * 4 + r0] * b.m[c * 4 + 2] +
                    a.m[3 * 4 + r0] * b.m[c * 4 + 3];
            }
        }
        return r;
    }

    // Right-handed lookAt
    static Mat4 lookAtRH(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = normalize(center - eye);
        Vec3 s = normalize(cross(f, up));
        Vec3 u = cross(s, f);

        Mat4 r = identity();
        r.m[0] = s.x; r.m[4] = s.y; r.m[8] = s.z;
        r.m[1] = u.x; r.m[5] = u.y; r.m[9] = u.z;
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;

        r.m[12] = -dot(s, eye);
        r.m[13] = -dot(u, eye);
        r.m[14] = dot(f, eye);
        return r;
    }

    // Perspective RH, depth 0..1 (как в Vulkan)
    static Mat4 perspectiveRH_ZO(float fovyRad, float aspect, float zNear, float zFar) {
        float f = 1.0f / std::tan(fovyRad * 0.5f);

        Mat4 r{};
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = zFar / (zNear - zFar);
        r.m[11] = -1.0f;
        r.m[14] = (zFar * zNear) / (zNear - zFar);
        return r;
    }

    static Mat4 translation(float x, float y, float z) {
        Mat4 r = identity();
        r.m[12] = x; r.m[13] = y; r.m[14] = z;
        return r;
    }

    static Mat4 scale(float x, float y, float z) {
        Mat4 r{};
        r.m[0] = x; r.m[5] = y; r.m[10] = z; r.m[15] = 1;
        return r;
    }

    static Mat4 rotationY(float a) {
        Mat4 r = identity();
        float c = std::cos(a), s = std::sin(a);
        r.m[0] = c;  r.m[8] = s;
        r.m[2] = -s; r.m[10] = c;
        return r;
    }

    static Mat4 rotationX(float a) {
        Mat4 r = identity();
        float c = std::cos(a), s = std::sin(a);
        r.m[5] = c;  r.m[9] = -s;
        r.m[6] = s;  r.m[10] = c;
        return r;
    }
};
