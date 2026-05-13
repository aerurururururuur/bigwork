#pragma once

#include "domain/enemy_bullet_sprite.hpp"

namespace domain {

class World;

/**
 * Reusable boss bullet geometry (no SkillCastContext): spawn only.
 * Call from skill.cpp or tests; keeps patterns decoupled from cast bookkeeping.
 */

/** Uniform fan [center - half, center + half], `count` bullets on arc. */
void boss_pattern_spawn_fan_sector(World& world, float cx, float cy, float angle_center_rad,
                                   float half_width_rad, int count, float bullet_speed, float muzzle_dist,
                                   int damage, EnemyBulletSprite sprite);

/** Ring on circle `radius`; velocity radial outward or inward (toward cx,cy). */
void boss_pattern_spawn_ring_radial(World& world, float cx, float cy, float radius, int count,
                                    float bullet_speed, bool velocity_outward, float muzzle_extra,
                                    int damage, EnemyBulletSprite sprite);

/**
 * One-shot Archimedean-style spiral: angle advances, radius grows, velocity tangential.
 * `clockwise` selects tangent direction.
 */
void boss_pattern_spawn_spiral_snapshot(World& world, float cx, float cy, int bullet_count, float turns,
                                        float radius_start, float radius_end, float tangent_speed,
                                        int damage, EnemyBulletSprite sprite, bool clockwise);

/** Two opposing 60deg fans aimed ~perpendicular to player (left/right). */
void boss_pattern_spawn_dual_opposing_fans(World& world, float cx, float cy, float aim_at_px,
                                           float aim_at_py, int count_per_fan, float half_width_rad,
                                           float bullet_speed, float muzzle_dist, int damage,
                                           EnemyBulletSprite sprite);

/** Outer ring outward + inner ring inward (crossing streams). */
void boss_pattern_spawn_cross_dual_ring(World& world, float cx, float cy, float radius_outer,
                                        float radius_inner, int count_outer, int count_inner,
                                        float speed_outer, float speed_inner, float muzzle_extra, int damage,
                                        EnemyBulletSprite sprite);

/** `count` directions around `aim_angle_rad` +/- `spread_half_rad`; soft homing after `straight_sec`. */
void boss_pattern_spawn_soft_scatter(World& world, float cx, float cy, float aim_angle_rad, int count,
                                       float spread_half_rad, float bullet_speed, double straight_sec,
                                       float max_turn_rad_per_sec, float muzzle_dist, int damage);

} // namespace domain
