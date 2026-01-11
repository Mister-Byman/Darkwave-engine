#include "game/Player.h"
#include <cmath>

static Vec3 normalizeXZ(const Vec3& v)
{
    float len = std::sqrt(v.x * v.x + v.z * v.z);
    if (len > 1e-6f) return { v.x / len, 0.0f, v.z / len };
    return { 0.0f, 0.0f, 0.0f };
}

void Player::update(float dt, const Vec3& wishDir, bool jumpPressed)
{
    grounded = false;

    // √оризонтальна€ скорость (пока без CS-ускорений Ч база)
    Vec3 dir = normalizeXZ(wishDir);
    velocity.x = dir.x * moveSpeed;
    velocity.z = dir.z * moveSpeed;

    // √равитаци€
    velocity.y -= gravity * dt;

    // »нтеграци€
    // ---- интеграци€ с коллизией (XZ) ----
    Vec3 oldPos = position;
    Vec3 newPos = position + (velocity * dt);

    // 1) пробуем пройти целиком
    position = newPos;

    // --- приземление на верх куба ---
    const float cubeTop = wallBox.max.y;

    // если падаем сверху
    if (velocity.y <= 0.0f)
    {
        float feetY = position.y;          // ноги игрока
        float prevFeetY = position.y - velocity.y * dt;

        // игрок был выше верха куба и пересЄк его
        bool crossedTop =
            prevFeetY >= cubeTop &&
            feetY <= cubeTop;

        // по XZ мы над кубом
        bool overCubeXZ =
            position.x > wallBox.min.x - radius &&
            position.x < wallBox.max.x + radius &&
            position.z > wallBox.min.z - radius &&
            position.z < wallBox.max.z + radius;

        if (crossedTop && overCubeXZ)
        {
            position.y = cubeTop;
            velocity.y = 0.0f;
            grounded = true;
        }
    }

    // —айд-коллизи€ работает только если "ноги" ниже верха куба
    if (position.y < cubeTop - 1e-3f)
    {
        if (resolveCircleAabbXZ(position, radius, wallBox))
        {
            Vec3 attempted = newPos - oldPos;
            Vec3 corrected = position - oldPos;

            if (fabsf(corrected.x - attempted.x) > 1e-4f) velocity.x = 0.0f;
            if (fabsf(corrected.z - attempted.z) > 1e-4f) velocity.z = 0.0f;
        }
    }

    // ≈сли уже стоим на кубе (почти ровно), держим grounded=true
    const bool overCubeXZ =
        (position.x >= wallBox.min.x) && (position.x <= wallBox.max.x) &&
        (position.z >= wallBox.min.z) && (position.z <= wallBox.max.z);

    if (overCubeXZ && fabsf(position.y - cubeTop) < 1e-3f && velocity.y <= 0.0f)
    {
        position.y = cubeTop;   // фиксируем от дрожани€
        velocity.y = 0.0f;
        grounded = true;
    }


    // ѕол: y >= 0
    if (position.y < 0.0f) {
        position.y = 0.0f;
        velocity.y = 0.0f;
        grounded = true;
    }

    // ѕрыжок должен быть после определени€ grounded в этом кадре
    if (jumpPressed && grounded) {
        velocity.y = jumpSpeed;
        grounded = false;
    }
}
