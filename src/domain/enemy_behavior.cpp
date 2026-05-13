#include "domain/enemy_behavior.hpp"

#include "domain/boss_hp_spell_constants.hpp"
#include "domain/combat_entities.hpp"
#include "domain/enemy_bullet_sprite.hpp"
#include "domain/enemy_engagement_constants.hpp"
#include "domain/enemy_sprite_id.hpp"
#include "domain/skill.hpp"
#include "domain/vec2.hpp"
#include "domain/wave_combat_tuning.hpp"
#include "domain/world.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

namespace domain {

namespace {

constexpr float kEnemyChaseManhattan = 10.f;
constexpr float kRangedHoldManhattan = 5.f;
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
            ports.melee->chasePlayerStep(self, world, dt, wave_combat::kEnemyChaseSpeed);
        } else {
            ports.melee->wanderStep(self, world, dt, wave_combat::kEnemyWanderSpeed, kEnemyChaseManhattan);
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
            ports.melee->wanderStep(self, world, dt, wave_combat::kEnemyWanderSpeed, kEnemyChaseManhattan);
            return;
        }
        if (ports.melee && md < kRangedHoldManhattan && md > 1e-3f) {
            float ax = self.x() - px;
            float ay = self.y() - py;
            normalizeOrDefault(ax, ay);
            self.integrateVelocity(world, ax * wave_combat::kRangedKiteSpeed, ay * wave_combat::kRangedKiteSpeed,
                                   static_cast<float>(dt));
        }
        if (ports.fire && self.enemyFireCoolRemaining() <= 0.0 &&
            world.lineOfSightClear(self.x(), self.y(), px, py)) {
            float bx = px - self.x();
            float by = py - self.y();
            normalizeOrDefault(bx, by);
            const float ox = self.x() + bx * kMuzzleOffset;
            const float oy = self.y() + by * kMuzzleOffset;
            ports.fire->spawnEnemyBullet(ox, oy, bx * wave_combat::kEnemyMobBulletSpeed,
                                         by * wave_combat::kEnemyMobBulletSpeed, wave_combat::kEnemyMobBulletDamage,
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
                ports.melee->wanderStep(self, world, dt, wave_combat::kEnemyWanderSpeed, kEnemyChaseManhattan);
            }
            return;
        }
        if (md <= kEliteHybridMeleeManhattanWorld) {
            if (ports.melee) {
                ports.melee->chasePlayerStep(self, world, dt, wave_combat::kEliteChaseSpeed);
            }
            return;
        }
        if (ports.melee && md < kRangedHoldManhattan && md > 1e-3f) {
            float ax = self.x() - px;
            float ay = self.y() - py;
            normalizeOrDefault(ax, ay);
            self.integrateVelocity(world, ax * wave_combat::kRangedKiteSpeed, ay * wave_combat::kRangedKiteSpeed,
                                   static_cast<float>(dt));
        }
        if (ports.fire && self.enemyFireCoolRemaining() <= 0.0 &&
            world.lineOfSightClear(self.x(), self.y(), px, py)) {
            float bx = px - self.x();
            float by = py - self.y();
            normalizeOrDefault(bx, by);
            const float ox = self.x() + bx * kMuzzleOffset;
            const float oy = self.y() + by * kMuzzleOffset;
            ports.fire->spawnEnemyBullet(ox, oy, bx * wave_combat::kEnemyMobBulletSpeed,
                                         by * wave_combat::kEnemyMobBulletSpeed, wave_combat::kEnemyMobBulletDamage,
                                         pickEnemyBulletSprite(self));
            self.armEnemyFireCooldown();
        }
    }
};

/**
 * Boss: stationary; early HP uses ring diffusion + fan alternation.
 * Mid/Late HP uses a spell-card sequence (windup -> dual ring -> dual fan -> soft scatter -> cross rings -> vulnerable).
 */
class BossHybridBehavior final : public IEnemyBehavior {
    enum class DiffusionPhase { None, Windup, WaitRing2 };
    enum class SpellCardPhase {
        IdleCombat,
        SpellWindup,
        R1WaitAfterRing1,
        PauseAfterRound1,
        PauseAfterRound2,
        PauseAfterScatter,
        Round3CrossRings,
        Vulnerable,
    };

    DiffusionPhase diffusion_{DiffusionPhase::None};
    double diffusion_rem_{0.0};
    double ring_burst_rem_{-1.0};
    bool next_attack_is_fan_{false};

    SpellCardPhase spell_{SpellCardPhase::IdleCombat};
    double spell_rem_{0.0};
    /** Tracks HP band to reset diffusion when crossing from early to mid/late. */
    bool was_early_hp_band_{true};

    static void armNextRingBurst(World& world, double& rem_out) {
        constexpr double kBaseSec = 5.0;
        constexpr double kMinSec = 2.5;
        const double jitter_ms = static_cast<double>(world.random().uniformInt(-400, 400)) / 1000.0;
        rem_out = kBaseSec + jitter_ms;
        if (rem_out < kMinSec) {
            rem_out = kMinSec;
        }
    }

    static float bossHpRatio(const EnemyActor& self) {
        const int m = std::max(1, self.maxHp());
        return static_cast<float>(self.hp()) / static_cast<float>(m);
    }

    void tickEarlyPattern(EnemyActor& self, CombatActorPorts ports, World& world, double dt) {
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

    void tickSpellSequence(EnemyActor& self, World& world, double dt) {
        SkillCastContext ctx{world, SkillCasterKind::Boss, nullptr, &self, 0.f, 0.f, 0, 1};

        switch (spell_) {
        case SpellCardPhase::SpellWindup:
            spell_rem_ -= dt;
            if (spell_rem_ <= 0.0) {
                bossDiffusionFireRing1(ctx);
                spell_ = SpellCardPhase::R1WaitAfterRing1;
                spell_rem_ = kBossSpellRing2DelaySec;
            }
            break;
        case SpellCardPhase::R1WaitAfterRing1:
            spell_rem_ -= dt;
            if (spell_rem_ <= 0.0) {
                bossDiffusionFireRing2(ctx);
                spell_ = SpellCardPhase::PauseAfterRound1;
                spell_rem_ = kBossSpellPauseShortSec;
            }
            break;
        case SpellCardPhase::PauseAfterRound1:
            spell_rem_ -= dt;
            if (spell_rem_ <= 0.0) {
                bossDualOpposingFanFire(ctx);
                spell_ = SpellCardPhase::PauseAfterRound2;
                spell_rem_ = kBossSpellPauseShortSec;
            }
            break;
        case SpellCardPhase::PauseAfterRound2:
            spell_rem_ -= dt;
            if (spell_rem_ <= 0.0) {
                bossSoftScatterFire(ctx);
                spell_ = SpellCardPhase::PauseAfterScatter;
                spell_rem_ = kBossSpellPauseShortSec;
            }
            break;
        case SpellCardPhase::PauseAfterScatter:
            spell_rem_ -= dt;
            if (spell_rem_ <= 0.0) {
                spell_ = SpellCardPhase::Round3CrossRings;
            }
            break;
        case SpellCardPhase::Round3CrossRings:
            bossCrossDualRingFire(ctx);
            spell_ = SpellCardPhase::Vulnerable;
            spell_rem_ = kBossSpellVulnerableSec;
            self.setIncomingDamageMultiplier(kBossSpellVulnerableDamageMult);
            break;
        case SpellCardPhase::Vulnerable:
            spell_rem_ -= dt;
            if (spell_rem_ <= 0.0) {
                self.setIncomingDamageMultiplier(1.f);
                spell_ = SpellCardPhase::IdleCombat;
                armNextRingBurst(world, ring_burst_rem_);
            }
            break;
        case SpellCardPhase::IdleCombat:
            break;
        }
    }

public:
    void tick(EnemyActor& self, CombatActorPorts ports, World& world, double dt) override {
        const float fdt = static_cast<float>(dt);
        self.integrateVelocity(world, 0.f, 0.f, fdt);

        const float hp_ratio = bossHpRatio(self);
        const bool now_early = hp_ratio > kBossHpRatioPhaseHigh;
        if (was_early_hp_band_ && !now_early) {
            diffusion_ = DiffusionPhase::None;
            diffusion_rem_ = 0.0;
            next_attack_is_fan_ = false;
        }
        was_early_hp_band_ = now_early;

        if (hp_ratio > kBossHpRatioPhaseHigh) {
            if (spell_ != SpellCardPhase::IdleCombat) {
                spell_ = SpellCardPhase::IdleCombat;
                spell_rem_ = 0.0;
                self.setIncomingDamageMultiplier(1.f);
            }
            tickEarlyPattern(self, ports, world, dt);
            return;
        }

        if (spell_ != SpellCardPhase::IdleCombat) {
            tickSpellSequence(self, world, dt);
            return;
        }

        if (ring_burst_rem_ < 0.0) {
            armNextRingBurst(world, ring_burst_rem_);
        }
        ring_burst_rem_ -= dt;
        if (ring_burst_rem_ <= 0.0 && ports.fire) {
            spell_ = SpellCardPhase::SpellWindup;
            spell_rem_ = kBossSpellWindupSec;
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
