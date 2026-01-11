#pragma once
#include "math/Vec3.h"

struct AABB
{
    Vec3 min;
    Vec3 max;
};

// circle (player) vs AABB in XZ plane
inline bool resolveCircleAabbXZ(Vec3& pos, float radius, const AABB& box)
{
    // Находим ближайшую точку на AABB к позиции круга (по XZ)
    float cx = pos.x;
    float cz = pos.z;

    float qx = cx;
    if (qx < box.min.x) qx = box.min.x;
    if (qx > box.max.x) qx = box.max.x;

    float qz = cz;
    if (qz < box.min.z) qz = box.min.z;
    if (qz > box.max.z) qz = box.max.z;

    float dx = cx - qx;
    float dz = cz - qz;
    float dist2 = dx * dx + dz * dz;

    float r = radius;
    float r2 = r * r;

    if (dist2 >= r2 || dist2 < 1e-12f) {
        // нет пересечения (или крайне редкий случай ровно в точке)
        return false;
    }

    float dist = sqrtf(dist2);
    float push = (r - dist);

    // нормаль от бокса к кругу
    float nx = dx / dist;
    float nz = dz / dist;

    // выталкиваем круг из бокса
    pos.x += nx * push;
    pos.z += nz * push;

    return true;
}
