#pragma once
#include "math/Vec3.h"
#include "game/Collision.h"

struct Player
{
    AABB wallBox{
    { -1.0f, 0.0f, -1.0f },
    {  1.0f, 2.0f,  1.0f }
    };

    Vec3 position{ 0.0f, 0.0f, 0.0f };
    Vec3 velocity{ 0.0f, 0.0f, 0.0f };

    float height = 1.8f;

    float radius = 0.30f;
    float eyeHeight = 1.7f;

    float moveSpeed = 4.5f;
    float jumpSpeed = 5.0f;
    float gravity = 9.81f;

    bool grounded = true;

    // wishDir — направление движения в МИРОВЫХ координатах (XZ), можно не нормализованное
    void update(float dt, const Vec3& wishDir, bool jumpPressed);
};
