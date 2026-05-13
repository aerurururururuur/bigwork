#include "domain/skill.hpp"

#include "domain/boss_pattern_spawn.hpp"
#include "domain/combat_entities.hpp"
#include "domain/enemy_bullet_sprite.hpp"
#include "domain/vec2.hpp"
#include "domain/world.hpp"

#include <cmath>

namespace domain {

namespace {

constexpr int kPlayerRingBulletCount = 28;
/** Keep in sync with `enemy_behavior.cpp` ranged / elite enemy shots. */
constexpr float kEnemyRingBulletSpeed = 9.0f;
constexpr float kEnemyRingMuzzleOffset = 0.42f;

constexpr int kBossDiffusionRing1Count = 12;
constexpr int kBossDiffusionRing2Count = 18;
/** Second ring rotated vs first (interleaved spokes). */
constexpr float kBossDiffusionRing2AngleOffset = 3.14159265358979323846f / 12.f;

/** Half-width each side (30 deg); total fan 60 deg. */
constexpr float kBossFanHalfWidthRad = 3.14159265358979323846f / 6.f;
/** Slower than ring burst (9) for dodge gaps. */
constexpr float kBossFanBulletSpeed = 7.0f;

/** Spiral one-shot tuning. */
constexpr int kBossSpiralBulletCount = 44;
constexpr float kBossSpiralTurns = 3.25f;
constexpr float kBossSpiralRadiusStart = 0.35f;
constexpr float kBossSpiralRadiusEnd = 2.8f;
constexpr float kBossSpiralTangentSpeed = 8.0f;

/** Dual fan: bullets per side, half-angle (rad). */
constexpr int kBossDualFanCountPerSide = 14;
constexpr float kBossDualFanHalfWidthRad = 3.14159265358979323846f / 7.f;

/** Cross rings. */
constexpr float kBossCrossRingOuter = 2.2f;
constexpr float kBossCrossRingInner = 1.05f;
constexpr int kBossCrossRingOuterCount = 16;
constexpr int kBossCrossRingInnerCount = 12;

/** Soft scatter. */
constexpr int kBossSoftScatterCountMin = 8;
constexpr int kBossSoftScatterCountMax = 10;
constexpr float kBossSoftScatterSpreadHalfRad = 3.14159265358979323846f / 10.f;
constexpr float kBossSoftScatterSpeed = 8.0f;
constexpr double kBossSoftScatterStraightSec = 0.6;
constexpr float kBossSoftScatterMaxTurnRps = 1.35f;

void spawnEnemyDiffusionRing(World& world, float cx, float cy, int n, float angle_offset_rad, float speed,
                             float muzzle, int damage, EnemyBulletSprite sprite) {
    for (int i = 0; i < n; ++i) {
        const float a =
            angle_offset_rad + (6.2831853071795864769f * static_cast<float>(i)) / static_cast<float>(n);
        float bx = std::cos(a);
        float by = std::sin(a);
        normalizeOrDefault(bx, by);
        world.spawnEnemyBullet(cx + bx * muzzle, cy + by * muzzle, bx * speed, by * speed, damage, sprite);
    }
}

} // namespace

int RingBurstSkill::mpCost() const {
    return 10;
}

double RingBurstSkill::cooldownSeconds() const {
    return 0.0;
}

void RingBurstSkill::execute(SkillCastContext& ctx) const {
    if (ctx.caster != SkillCasterKind::Player) {
        return;
    }
    const float cx = ctx.player_foot_x;
    const float cy = ctx.player_foot_y - player_shot::kFeetToCenterWorld;
    for (int i = 0; i < kPlayerRingBulletCount; ++i) {
        const float a =
            (6.2831853071795864769f * static_cast<float>(i)) / static_cast<float>(kPlayerRingBulletCount);
        float bx = std::cos(a);
        float by = std::sin(a);
        normalizeOrDefault(bx, by);
        const float ox = cx + bx * player_shot::kMuzzleOffsetWorld;
        const float oy = cy + by * player_shot::kMuzzleOffsetWorld;
        ctx.world.spawnPlayerBullet(ox, oy, bx * player_shot::kBulletSpeed, by * player_shot::kBulletSpeed,
                                    ctx.bullet_damage);
    }
}

const ISkill& ringBurstSkill() {
    static const RingBurstSkill k{};
    return k;
}

int BossRingBurstSkill::mpCost() const {
    return 0;
}

double BossRingBurstSkill::cooldownSeconds() const {
    return 0.0;
}

void BossRingBurstSkill::execute(SkillCastContext& ctx) const {
    bossDiffusionFireRing1(ctx);
}

const ISkill& bossRingBurstSkill() {
    static const BossRingBurstSkill k{};
    return k;
}

void bossDiffusionFireRing1(SkillCastContext& ctx) {
    if (ctx.caster != SkillCasterKind::Boss || ctx.enemy == nullptr) {
        return;
    }
    const float cx = ctx.enemy->x();
    const float cy = ctx.enemy->y();
    spawnEnemyDiffusionRing(ctx.world, cx, cy, kBossDiffusionRing1Count, 0.f, kEnemyRingBulletSpeed,
                            kEnemyRingMuzzleOffset, ctx.enemy_bullet_damage, EnemyBulletSprite::PebblinRock);
}

void bossDiffusionFireRing2(SkillCastContext& ctx) {
    if (ctx.caster != SkillCasterKind::Boss || ctx.enemy == nullptr) {
        return;
    }
    const float cx = ctx.enemy->x();
    const float cy = ctx.enemy->y();
    spawnEnemyDiffusionRing(ctx.world, cx, cy, kBossDiffusionRing2Count, kBossDiffusionRing2AngleOffset,
                            kEnemyRingBulletSpeed, kEnemyRingMuzzleOffset, ctx.enemy_bullet_damage,
                            EnemyBulletSprite::PebblinRock);
}

void bossFanBarrageFire(SkillCastContext& ctx) {
    if (ctx.caster != SkillCasterKind::Boss || ctx.enemy == nullptr) {
        return;
    }
    const float ex = ctx.enemy->x();
    const float ey = ctx.enemy->y();
    const float px = ctx.world.player().x();
    const float py = ctx.world.player().y();
    const float dx = px - ex;
    const float dy = py - ey;
    float theta = 0.f;
    if (lengthSq(dx, dy) > kEpsilon * kEpsilon) {
        theta = std::atan2(dy, dx);
    }
    const int n = ctx.world.random().uniformInt(20, 28);
    boss_pattern_spawn_fan_sector(ctx.world, ex, ey, theta, kBossFanHalfWidthRad, n, kBossFanBulletSpeed,
                                  kEnemyRingMuzzleOffset, ctx.enemy_bullet_damage, EnemyBulletSprite::PebblinRock);
}

void bossSpiralBurstFire(SkillCastContext& ctx) {
    if (ctx.caster != SkillCasterKind::Boss || ctx.enemy == nullptr) {
        return;
    }
    const float cx = ctx.enemy->x();
    const float cy = ctx.enemy->y();
    const bool clockwise = (ctx.world.random().uniformInt(0, 1) != 0);
    boss_pattern_spawn_spiral_snapshot(ctx.world, cx, cy, kBossSpiralBulletCount, kBossSpiralTurns,
                                       kBossSpiralRadiusStart, kBossSpiralRadiusEnd, kBossSpiralTangentSpeed,
                                       ctx.enemy_bullet_damage, EnemyBulletSprite::PebblinRock, clockwise);
}

void bossDualOpposingFanFire(SkillCastContext& ctx) {
    if (ctx.caster != SkillCasterKind::Boss || ctx.enemy == nullptr) {
        return;
    }
    const float cx = ctx.enemy->x();
    const float cy = ctx.enemy->y();
    const float px = ctx.world.player().x();
    const float py = ctx.world.player().y();
    boss_pattern_spawn_dual_opposing_fans(ctx.world, cx, cy, px, py, kBossDualFanCountPerSide,
                                          kBossDualFanHalfWidthRad, kBossFanBulletSpeed, kEnemyRingMuzzleOffset,
                                          ctx.enemy_bullet_damage, EnemyBulletSprite::PebblinRock);
}

void bossCrossDualRingFire(SkillCastContext& ctx) {
    if (ctx.caster != SkillCasterKind::Boss || ctx.enemy == nullptr) {
        return;
    }
    const float cx = ctx.enemy->x();
    const float cy = ctx.enemy->y();
    boss_pattern_spawn_cross_dual_ring(ctx.world, cx, cy, kBossCrossRingOuter, kBossCrossRingInner,
                                       kBossCrossRingOuterCount, kBossCrossRingInnerCount, kEnemyRingBulletSpeed,
                                       kBossFanBulletSpeed, kEnemyRingMuzzleOffset, ctx.enemy_bullet_damage,
                                       EnemyBulletSprite::PebblinRock);
}

void bossSoftScatterFire(SkillCastContext& ctx) {
    if (ctx.caster != SkillCasterKind::Boss || ctx.enemy == nullptr) {
        return;
    }
    const float cx = ctx.enemy->x();
    const float cy = ctx.enemy->y();
    const float px = ctx.world.player().x();
    const float py = ctx.world.player().y();
    float dx = px - cx;
    float dy = py - cy;
    float aim = 0.f;
    if (lengthSq(dx, dy) > kEpsilon * kEpsilon) {
        aim = std::atan2(dy, dx);
    }
    const int n = ctx.world.random().uniformInt(kBossSoftScatterCountMin, kBossSoftScatterCountMax);
    boss_pattern_spawn_soft_scatter(ctx.world, cx, cy, aim, n, kBossSoftScatterSpreadHalfRad, kBossSoftScatterSpeed,
                                    kBossSoftScatterStraightSec, kBossSoftScatterMaxTurnRps, kEnemyRingMuzzleOffset,
                                    ctx.enemy_bullet_damage);
}

} // namespace domain
