#include "game/CameraFPS.h"
#include <cmath>

void CameraFPS::updateLook(int dx, int dy) {
	yaw_ += (float)dx * sens_;
	pitch_ += (float)-dy * sens_; // инверси€ Y: мышь вверх -> смотреть вверх

	// ограничиваем pitch чтобы не переворачиватьс€
	const float limit = 1.553343f; // ~89 degrees in radians
	pitch_ = clamp(pitch_, -limit, limit);
}

Vec3 CameraFPS::forward() const {
	// yaw вокруг Y, pitch вокруг X (в мировой системе Y-up)
	float cy = std::cos(yaw_);
	float sy = std::sin(yaw_);
	float cp = std::cos(pitch_);
	float sp = std::sin(pitch_);

	// классический FPS forward
	Vec3 f{ sy * cp, sp, -cy * cp };
	return normalize(f);
}

Vec3 CameraFPS::right() const {
	// right = normalize(cross(forward, up))
	return normalize(cross(forward(), { 0,1,0 }));
}

void CameraFPS::updateMove(float forwardAxis, float rightAxis, float upAxis, float dt) {
	Vec3 f = forward();
	// чтобы WASD не летал вверх/вниз из-за pitch Ч обнулим Y дл€ движени€ по земле
	Vec3 fFlat = normalize(Vec3{ f.x, 0.0f, f.z });
	Vec3 r = right();

	Vec3 vel = fFlat * forwardAxis + r * rightAxis + Vec3{ 0,1,0 } *upAxis;
	pos_ += vel * (speed_ * dt);
}
