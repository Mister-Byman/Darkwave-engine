#pragma once
#include <cmath>

struct Vec3 {
	float x = 0, y = 0, z = 0;

	Vec3() = default;
	Vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}

	Vec3 operator+(const Vec3& r) const { return { x + r.x, y + r.y, z + r.z }; }
	Vec3 operator-(const Vec3& r) const { return { x - r.x, y - r.y, z - r.z }; }
	Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }

	Vec3& operator+=(const Vec3& r) { x += r.x; y += r.y; z += r.z; return *this; }
};

inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 cross(const Vec3& a, const Vec3& b) {
	return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

inline float length(const Vec3& v) { return std::sqrt(dot(v, v)); }

inline Vec3 normalize(const Vec3& v) {
	float len = length(v);
	if (len <= 1e-6f) return { 0,0,0 };
	return v * (1.0f / len);
}
