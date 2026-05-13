#include "domain/boss_pattern_spawn.hpp"

#include "domain/vec2.hpp"
#include "domain/world.hpp"

#include <algorithm>
#include <cmath>

namespace domain {

namespace {

constexpr float kTwoPi = 6.2831853071795864769f;

} // namespace

void boss_pattern_spawn_fan_sector(World& world, float cx, float cy, float angle_center_rad,
                                   float half_width_rad, int count, float bullet_speed, float muzzle_dist,
                                   int damage, EnemyBulletSprite sprite, std::uint8_t boss_bullet_strip) {
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
                               by * bullet_speed, damage, sprite, boss_bullet_strip);
    }
}

void boss_pattern_spawn_ring_radial(World& world, float cx, float cy, float radius, int count,
                                    float bullet_speed, bool velocity_outward, float muzzle_extra,
                                    int damage, EnemyBulletSprite sprite, std::uint8_t boss_bullet_strip) {
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
        world.spawnEnemyBullet(px, py, vx * bullet_speed, vy * bullet_speed, damage, sprite, boss_bullet_strip);
    }
}

void boss_pattern_spawn_spiral_snapshot(World& world, float cx, float cy, int bullet_count, float turns,
                                        float radius_start, float radius_end, float tangent_speed,
                                        int damage, EnemyBulletSprite sprite, bool clockwise,
                                        std::uint8_t boss_bullet_strip) {
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
        world.spawnEnemyBullet(ox, oy, tx * tangent_speed, ty * tangent_speed, damage, sprite, boss_bullet_strip);
    }
}

void boss_pattern_spawn_dual_opposing_fans(World& world, float cx, float cy, float aim_at_px,
                                           float aim_at_py, int count_per_fan, float half_width_rad,
                                           float bullet_speed, float muzzle_dist, int damage,
                                           EnemyBulletSprite sprite, std::uint8_t boss_bullet_strip) {
    float dx = aim_at_px - cx;
    float dy = aim_at_py - cy;
    float theta = 0.f;
    if (lengthSq(dx, dy) > kEpsilon * kEpsilon) {
        theta = std::atan2(dy, dx);
    }
    constexpr float kHalfPi = 1.57079632679489661923f;
    boss_pattern_spawn_fan_sector(world, cx, cy, theta + kHalfPi, half_width_rad, count_per_fan, bullet_speed,
                                  muzzle_dist, damage, sprite, boss_bullet_strip);
    boss_pattern_spawn_fan_sector(world, cx, cy, theta - kHalfPi, half_width_rad, count_per_fan, bullet_speed,
                                  muzzle_dist, damage, sprite, boss_bullet_strip);
}

void boss_pattern_spawn_cross_dual_ring(World& world, float cx, float cy, float radius_outer,
                                        float radius_inner, int count_outer, int count_inner,
                                        float speed_outer, float speed_inner, float muzzle_extra, int damage,
                                        EnemyBulletSprite sprite, std::uint8_t boss_bullet_strip) {
    boss_pattern_spawn_ring_radial(world, cx, cy, radius_outer, count_outer, speed_outer, true, muzzle_extra,
                                   damage, sprite, boss_bullet_strip);
    boss_pattern_spawn_ring_radial(world, cx, cy, radius_inner, count_inner, speed_inner, false, muzzle_extra,
                                   damage, sprite, boss_bullet_strip);
}

void boss_pattern_spawn_soft_scatter(World& world, float cx, float cy, float aim_angle_rad, int count,
                                       float spread_half_rad, float bullet_speed, double straight_sec,
                                       float max_turn_rad_per_sec, float muzzle_dist, int damage,
                                       EnemyBulletSprite sprite, std::uint8_t boss_bullet_strip) {
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
                                         by * bullet_speed, damage, sprite, straight_sec, max_turn_rad_per_sec,
                                         boss_bullet_strip);
    }
}

void boss_pattern_spawn_wall_volley(World& world, float player_x, float boss_x, int playfield_width_cells,
                                    int playfield_height_cells, int bullet_count, float bullet_speed,
                                    float wall_inset, float y_margin, int damage, EnemyBulletSprite sprite,
                                    std::uint8_t boss_bullet_strip) {
    if (bullet_count <= 0 || playfield_width_cells <= 0 || playfield_height_cells <= 0) {
        return;
    }
    const float hf = static_cast<float>(playfield_height_cells);
    const float y0 = std::max(wall_inset, y_margin);
    const float y1 = std::min(hf - wall_inset, hf - y_margin);
    if (y1 <= y0 + 1e-4f) {
        return;
    }
    const bool from_left = (player_x >= boss_x);
    // Spawn just inside walkable columns (0 and width-1 are walls; same convention as `World` mob spawn).
    const float inner_left_x = 1.f + wall_inset;
    const float inner_right_x = static_cast<float>(playfield_width_cells - 2) + wall_inset;
    const float spawn_x = from_left ? inner_left_x : inner_right_x;
    const float vx = from_left ? bullet_speed : -bullet_speed;
    for (int i = 0; i < bullet_count; ++i) {
        const float t =
            (bullet_count <= 1) ? 0.5f : static_cast<float>(i) / static_cast<float>(bullet_count - 1);
        const float y = y0 + t * (y1 - y0);
        world.spawnEnemyBullet(spawn_x, y, vx, 0.f, damage, sprite, boss_bullet_strip);
    }
}

} // namespace domain
