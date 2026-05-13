#include "domain/enemy_behavior.hpp"

#include "domain/combat_entities.hpp"
#include "domain/enemy_bullet_sprite.hpp"
#include "domain/enemy_engagement_constants.hpp"
#include "domain/enemy_sprite_id.hpp"
#include "domain/skill.hpp"
#include "domain/vec2.hpp"
#include "domain/world.hpp"

#include <cmath>
#include <memory>

namespace domain {

namespace {

constexpr float kEnemyChaseManhattan = 10.f;
constexpr float kRangedHoldManhattan = 5.f;
constexpr float kEnemyChaseSpeed = 2.8f;
constexpr float kEnemyWanderSpeed = 1.0f;
constexpr float kRangedKiteSpeed = 2.2f;
constexpr float kEliteChaseSpeed = 3.35f;
constexpr float kBulletSpeed = 9.0f;
constexpr float kMuzzleOffset = 0.42f;

inline float manhattan(float ax, float ay, float bx, float by) {
    return std::abs(ax - bx) + std::abs(ay - by);
}

inline EnemyBulletSprite pickEnemyBulletSprite(const EnemyActor& self) {
    return self.spriteId() == EnemySpriteId::Pebblin ? EnemyBulletSprite::PebblinRock
                                                    : EnemyBulletSprite::Generic;
}

} // namespace

class MeleeChaseBehavior final : public IEnemyBehavior {
public:
    void tick(EnemyActor& self, CombatActorPorts ports, World& world, double dt) override {
        if (!ports.melee) {
            return;
        }
        const float md = manhattan(self.x(), self.y(), world.player().x(), world.player().y());
        if (md <= kEnemyChaseManhattan) {
            ports.melee->chasePlayerStep(self, world, dt, kEnemyChaseSpeed);
        } else {
            ports.melee->wanderStep(self, world, dt, kEnemyWanderSpeed, kEnemyChaseManhattan);
        }
    }
};

class RangedKiteBehavior final : public IEnemyBehavior {
public:
    void tick(EnemyActor& self, CombatActorPorts ports, World& world, double dt) override {
        self.advanceEnemyFireCooldown(dt);
        const float px = world.player().x();
        const float py = world.player().y();
        const float md = manhattan(self.x(), self.y(), px, py);

        if (ports.melee && md > kEnemyChaseManhattan) {
            ports.melee->wanderStep(self, world, dt, kEnemyWanderSpeed, kEnemyChaseManhattan);
            return;
        }
        if (ports.melee && md < kRangedHoldManhattan && md > 1e-3f) {
            float ax = self.x() - px;
            float ay = self.y() - py;
            normalizeOrDefault(ax, ay);
            self.integrateVelocity(world, ax * kRangedKiteSpeed, ay * kRangedKiteSpeed,
                                   static_cast<float>(dt));
        }
        if (ports.fire && self.enemyFireCoolRemaining() <= 0.0 &&
            world.lineOfSightClear(self.x(), self.y(), px, py)) {
            float bx = px - self.x();
            float by = py - self.y();
            normalizeOrDefault(bx, by);
            const float ox = self.x() + bx * kMuzzleOffset;
            const float oy = self.y() + by * kMuzzleOffset;
            ports.fire->spawnEnemyBullet(ox, oy, bx * kBulletSpeed, by * kBulletSpeed, 1,
                                         pickEnemyBulletSprite(self));
            self.armEnemyFireCooldown();
        }
    }
};

class EliteHybridBehavior : public IEnemyBehavior {
public:
    void tick(EnemyActor& self, CombatActorPorts ports, World& world, double dt) override {
        self.advanceEnemyFireCooldown(dt);
        const float px = world.player().x();
        const float py = world.player().y();
        const float md = manhattan(self.x(), self.y(), px, py);

        if (md > kEnemyChaseManhattan) {
            if (ports.melee) {
                ports.melee->wanderStep(self, world, dt, kEnemyWanderSpeed, kEnemyChaseManhattan);
            }
            return;
        }
        if (md <= kEliteHybridMeleeManhattanWorld) {
            if (ports.melee) {
                ports.melee->chasePlayerStep(self, world, dt, kEliteChaseSpeed);
            }
            return;
        }
        if (ports.melee && md < kRangedHoldManhattan && md > 1e-3f) {
            float ax = self.x() - px;
            float ay = self.y() - py;
            normalizeOrDefault(ax, ay);
            self.integrateVelocity(world, ax * kRangedKiteSpeed, ay * kRangedKiteSpeed,
                                   static_cast<float>(dt));
        }
        if (ports.fire && self.enemyFireCoolRemaining() <= 0.0 &&
            world.lineOfSightClear(self.x(), self.y(), px, py)) {
            float bx = px - self.x();
            float by = py - self.y();
            normalizeOrDefault(bx, by);
            const float ox = self.x() + bx * kMuzzleOffset;
            const float oy = self.y() + by * kMuzzleOffset;
            ports.fire->spawnEnemyBullet(ox, oy, bx * kBulletSpeed, by * kBulletSpeed, 1,
                                         pickEnemyBulletSprite(self));
            self.armEnemyFireCooldown();
        }
    }
};

/**
 * Boss: elite hybrid combat, plus Touhou-style ring diffusion (~5s + jitter):
 * 1s standstill windup, 12 bullets, 0.5s delay, 18 bullets with angle offset (rock sprites).
 * Alternates with a one-shot fan barrage (aim at player, +-30 deg, 20-28 rocks).
 */
class BossHybridBehavior final : public EliteHybridBehavior {
    enum class DiffusionPhase { None, Windup, WaitRing2 };

    DiffusionPhase diffusion_{DiffusionPhase::None};
    /** Time remaining in current diffusion phase (windup or ring2 delay). */
    double diffusion_rem_{0.0};
    /** Seconds until next boss pattern; `<0` = not yet armed. */
    double ring_burst_rem_{-1.0};
    /** After a full ring sequence, next timer triggers fan instead of windup. */
    bool next_attack_is_fan_{false};

    static void armNextRingBurst(World& world, double& rem_out) {
        constexpr double kBaseSec = 5.0;
        constexpr double kMinSec = 2.5;
        const double jitter_ms = static_cast<double>(world.random().uniformInt(-400, 400)) / 1000.0;
        rem_out = kBaseSec + jitter_ms;
        if (rem_out < kMinSec) {
            rem_out = kMinSec;
        }
    }

public:
    void tick(EnemyActor& self, CombatActorPorts ports, World& world, double dt) override {
        if (diffusion_ != DiffusionPhase::None) {
            diffusion_rem_ -= dt;
            if (diffusion_ == DiffusionPhase::Windup) {
                if (diffusion_rem_ <= 0.0) {
                    SkillCastContext ctx{world, SkillCasterKind::Boss, nullptr, &self, 0.f, 0.f, 0, 1};
                    bossDiffusionFireRing1(ctx);
                    diffusion_ = DiffusionPhase::WaitRing2;
                    diffusion_rem_ = 0.5;
                }
                return;
            }
            if (diffusion_ == DiffusionPhase::WaitRing2) {
                if (diffusion_rem_ <= 0.0) {
                    SkillCastContext ctx{world, SkillCasterKind::Boss, nullptr, &self, 0.f, 0.f, 0, 1};
                    bossDiffusionFireRing2(ctx);
                    diffusion_ = DiffusionPhase::None;
                    armNextRingBurst(world, ring_burst_rem_);
                    next_attack_is_fan_ = true;
                }
                return;
            }
        }

        EliteHybridBehavior::tick(self, ports, world, dt);

        if (ring_burst_rem_ < 0.0) {
            armNextRingBurst(world, ring_burst_rem_);
        }
        ring_burst_rem_ -= dt;
        if (ring_burst_rem_ <= 0.0 && ports.fire) {
            if (next_attack_is_fan_) {
                SkillCastContext ctx{world, SkillCasterKind::Boss, nullptr, &self, 0.f, 0.f, 0, 1};
                bossFanBarrageFire(ctx);
                armNextRingBurst(world, ring_burst_rem_);
                next_attack_is_fan_ = false;
            } else {
                diffusion_ = DiffusionPhase::Windup;
                diffusion_rem_ = 1.0;
            }
        }
    }
};

std::unique_ptr<IEnemyBehavior> makeEnemyBehavior(EnemyArchetype a) {
    switch (a) {
    case EnemyArchetype::Melee:
        return std::make_unique<MeleeChaseBehavior>();
    case EnemyArchetype::Ranged:
        return std::make_unique<RangedKiteBehavior>();
    case EnemyArchetype::EliteHybrid:
        return std::make_unique<EliteHybridBehavior>();
    case EnemyArchetype::Boss:
        return std::make_unique<BossHybridBehavior>();
    default:
        return std::make_unique<MeleeChaseBehavior>();
    }
}

} // namespace domain
