#include "domain/boss_pattern_spawn.hpp"

#include "domain/vec2.hpp"
#include "domain/world.hpp"

#include <cmath>

namespace domain {

namespace {

constexpr float kTwoPi = 6.2831853071795864769f;

} // namespace

void boss_pattern_spawn_fan_sector(World& world, float cx, float cy, float angle_center_rad,
                                   float half_width_rad, int count, float bullet_speed, float muzzle_dist,
                                   int damage, EnemyBulletSprite sprite) {
    if (count <= 0) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        const float t = (count <= 1) ? 0.5f : static_cast<float>(i) / static_cast<float>(count - 1);
        const float a = (angle_center_rad - half_width_rad) + t * (2.f * half_width_rad);
        float bx = std::cos(a);
        float by = std::sin(a);
        normalizeOrDefault(bx, by);
        world.spawnEnemyBullet(cx + bx * muzzle_dist, cy + by * muzzle_dist, bx * bullet_speed,
                               by * bullet_speed, damage, sprite);
    }
}

void boss_pattern_spawn_ring_radial(World& world, float cx, float cy, float radius, int count,
                                    float bullet_speed, bool velocity_outward, float muzzle_extra,
                                    int damage, EnemyBulletSprite sprite) {
    if (count <= 0 || radius <= 1e-4f) {
        return;
    }
    const float rspawn = radius + muzzle_extra;
    for (int i = 0; i < count; ++i) {
        const float a = (kTwoPi * static_cast<float>(i)) / static_cast<float>(count);
        const float px = cx + std::cos(a) * rspawn;
        const float py = cy + std::sin(a) * rspawn;
        float vx = std::cos(a);
        float vy = std::sin(a);
        if (!velocity_outward) {
            vx = -vx;
            vy = -vy;
        }
        normalizeOrDefault(vx, vy);
        world.spawnEnemyBullet(px, py, vx * bullet_speed, vy * bullet_speed, damage, sprite);
    }
}

void boss_pattern_spawn_spiral_snapshot(World& world, float cx, float cy, int bullet_count, float turns,
                                        float radius_start, float radius_end, float tangent_speed,
                                        int damage, EnemyBulletSprite sprite, bool clockwise) {
    if (bullet_count <= 0) {
        return;
    }
    const float span = std::max(0.05f, turns) * kTwoPi;
    for (int i = 0; i < bullet_count; ++i) {
        const float u = (bullet_count <= 1) ? 0.5f : static_cast<float>(i) / static_cast<float>(bullet_count - 1);
        const float phi = u * span;
        const float r = radius_start + u * (radius_end - radius_start);
        const float ox = cx + std::cos(phi) * r;
        const float oy = cy + std::sin(phi) * r;
        float tx = -std::sin(phi);
        float ty = std::cos(phi);
        if (clockwise) {
            tx = -tx;
            ty = -ty;
        }
        normalizeOrDefault(tx, ty);
        world.spawnEnemyBullet(ox, oy, tx * tangent_speed, ty * tangent_speed, damage, sprite);
    }
}

void boss_pattern_spawn_dual_opposing_fans(World& world, float cx, float cy, float aim_at_px,
                                           float aim_at_py, int count_per_fan, float half_width_rad,
                                           float bullet_speed, float muzzle_dist, int damage,
                                           EnemyBulletSprite sprite) {
    float dx = aim_at_px - cx;
    float dy = aim_at_py - cy;
    float theta = 0.f;
    if (lengthSq(dx, dy) > kEpsilon * kEpsilon) {
        theta = std::atan2(dy, dx);
    }
    constexpr float kHalfPi = 1.57079632679489661923f;
    boss_pattern_spawn_fan_sector(world, cx, cy, theta + kHalfPi, half_width_rad, count_per_fan, bullet_speed,
                                  muzzle_dist, damage, sprite);
    boss_pattern_spawn_fan_sector(world, cx, cy, theta - kHalfPi, half_width_rad, count_per_fan, bullet_speed,
                                  muzzle_dist, damage, sprite);
}

void boss_pattern_spawn_cross_dual_ring(World& world, float cx, float cy, float radius_outer,
                                        float radius_inner, int count_outer, int count_inner,
                                        float speed_outer, float speed_inner, float muzzle_extra, int damage,
                                        EnemyBulletSprite sprite) {
    boss_pattern_spawn_ring_radial(world, cx, cy, radius_outer, count_outer, speed_outer, true, muzzle_extra,
                                   damage, sprite);
    boss_pattern_spawn_ring_radial(world, cx, cy, radius_inner, count_inner, speed_inner, false, muzzle_extra,
                                   damage, sprite);
}

void boss_pattern_spawn_soft_scatter(World& world, float cx, float cy, float aim_angle_rad, int count,
                                       float spread_half_rad, float bullet_speed, double straight_sec,
                                       float max_turn_rad_per_sec, float muzzle_dist, int damage) {
    if (count <= 0 || straight_sec < 0.0) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        const float t = (count <= 1) ? 0.5f : static_cast<float>(i) / static_cast<float>(count - 1);
        const float a = (aim_angle_rad - spread_half_rad) + t * (2.f * spread_half_rad);
        float bx = std::cos(a);
        float by = std::sin(a);
        normalizeOrDefault(bx, by);
        world.spawnEnemyBulletSoftHoming(cx + bx * muzzle_dist, cy + by * muzzle_dist, bx * bullet_speed,
                                         by * bullet_speed, damage, EnemyBulletSprite::PebblinRock, straight_sec,
                                         max_turn_rad_per_sec);
    }
}

} // namespace domain
