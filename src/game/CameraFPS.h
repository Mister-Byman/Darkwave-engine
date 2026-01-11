#pragma once
#include "math/Vec3.h"

class CameraFPS {
public:
    void setPosition(const Vec3& p) { pos_ = p; }
    const Vec3& position() const { return pos_; }

    float yaw() const { return yaw_; }     // radians
    float pitch() const { return pitch_; } // radians

    void setMouseSensitivity(float s) { sens_ = s; } // radians per pixel
    void setMoveSpeed(float s) { speed_ = s; }

    // dx/dy: relative mouse, dt: seconds
    void updateLook(int dx, int dy);
    // move: [-1..1] forward/right/up, dt seconds
    void updateMove(float forward, float right, float up, float dt);

    Vec3 forward() const;
    Vec3 right() const;
    Vec3 up() const { return { 0,1,0 }; }

private:
    Vec3 pos_{ 0, 1.7f, 3.0f };

    float yaw_ = 0.0f;     // вокруг Y
    float pitch_ = 0.0f;   // вверх/вниз

    float sens_ = 0.0025f; // ~ комфортно
    float speed_ = 4.0f;   // м/с условные

    static float clamp(float v, float lo, float hi) {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }
};
