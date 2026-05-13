#include "domain/combat_entities.hpp"

#include "domain/bullet_hit_policy.hpp"
#include "domain/enemy_behavior.hpp"
#include "domain/skill.hpp"
#include "domain/vec2.hpp"
#include "domain/wave_combat_tuning.hpp"
#include "domain/world.hpp"

#include <algorithm>
#include <cmath>

namespace domain {

namespace {

constexpr float kPlayerSpeed = 8.4f;
constexpr double kPlayerMpRegenPerSec = 2.75;

} // namespace

BulletActor::BulletActor(float x, float y, float vx, float vy, int damage,
                         std::unique_ptr<IBulletHitPolicy> policy)
    : x_(x), y_(y), vx_(vx), vy_(vy), damage_(damage), hit_policy_(std::move(policy)) {}

BulletActor::~BulletActor() = default;

PlayerBulletActor::PlayerBulletActor(float x, float y, float vx, float vy, int damage, float max_travel_sq,
                                     std::uint8_t player_bullet_visual)
    : BulletActor(x, y, vx, vy, damage, makeBulletHitPolicy(BulletFaction::Player)),
      spawn_x_(x),
      spawn_y_(y),
      max_travel_sq_(max_travel_sq),
      player_bullet_visual_(player_bullet_visual) {}

void PlayerBulletActor::integratePosition(World& world, float fdt) {
    BulletActor::integratePosition(world, fdt);
    if (max_travel_sq_ > 0.f) {
        const float dx = x_ - spawn_x_;
        const float dy = y_ - spawn_y_;
        if (lengthSq(dx, dy) > max_travel_sq_) {
            dead_ = true;
        }
    }
}

EnemyBulletActor::EnemyBulletActor(float x, float y, float vx, float vy, int damage, EnemyBulletSprite sprite,
                                   std::uint8_t boss_bullet_strip, double soft_homing_straight_sec,
                                   float max_turn_rad_per_sec, float max_travel_sq)
    : BulletActor(x, y, vx, vy, damage, makeBulletHitPolicy(BulletFaction::Enemy)),
      sprite_(sprite),
      boss_bullet_strip_(boss_bullet_strip),
      soft_homing_(soft_homing_straight_sec >= 0.0),
      straight_rem_(soft_homing_ ? soft_homing_straight_sec : 0.0),
      max_turn_rad_per_sec_(soft_homing_ && max_turn_rad_per_sec > 1e-5f ? max_turn_rad_per_sec : 1.2f),
      spawn_x_(x),
      spawn_y_(y),
      max_travel_sq_(max_travel_sq) {
    const float sp = length(vx, vy);
    speed_scalar_ = sp > kEpsilon ? sp : 7.0f;
}

void EnemyBulletActor::checkMaxTravelAfterStep() {
    if (max_travel_sq_ <= 0.f) {
        return;
    }
    const float dx = x_ - spawn_x_;
    const float dy = y_ - spawn_y_;
    if (lengthSq(dx, dy) > max_travel_sq_) {
        dead_ = true;
    }
}

EnemyActor::~EnemyActor() = default;

bool PlayerActor::destroyed() const noexcept {
    return hp_ <= 0;
}

void PlayerActor::resetSkillState() {
    skill_slot_cd_.fill(0.0);
    skill_anim_rem_ = 0.0;
    skill_anim_total_ = 0.0;
}

void PlayerActor::healHp(int delta) {
    if (delta <= 0) {
        return;
    }
    hp_ = std::min(max_hp_, hp_ + delta);
}

void PlayerActor::restoreMp(int delta) {
    if (delta <= 0) {
        return;
    }
    mp_ = std::min(max_mp_, mp_ + delta);
}

void PlayerActor::step(World& world, double dt) {
    bullet_invuln_rem_ = std::max(0.0, bullet_invuln_rem_ - dt);
    fire_cool_ -= dt;

    mp_regen_carry_ += kPlayerMpRegenPerSec * dt;
    const int gain = static_cast<int>(mp_regen_carry_);
    mp_regen_carry_ -= static_cast<double>(gain);
    mp_ = std::min(max_mp_, mp_ + gain);

    const PlayerIntent& in = world.frameIntent();
    const float fdt = static_cast<float>(dt);

    if (in.move_dx != 0 || in.move_dy != 0) {
        last_fire_dx_ = in.move_dx;
        last_fire_dy_ = in.move_dy;
    }

    float mx = static_cast<float>(in.move_dx);
    float my = static_cast<float>(in.move_dy);
    if (mx != 0.f || my != 0.f) {
        normalizeOrDefault(mx, my);
        vx_ = mx * kPlayerSpeed;
        vy_ = my * kPlayerSpeed;
    } else {
        vx_ = 0.f;
        vy_ = 0.f;
    }

    const float nx = x_ + vx_ * fdt;
    const float ny = y_ + vy_ * fdt;

    if (world.playerFitsAt(nx, ny)) {
        x_ = nx;
        y_ = ny;
    } else if (world.playerFitsAt(nx, y_)) {
        x_ = nx;
    } else if (world.playerFitsAt(x_, ny)) {
        y_ = ny;
    }

    if (in.fire && fire_cool_ <= 0.0) {
        float bx = 0.f;
        float by = 0.f;
        if (in.use_mouse_aim) {
            bx = in.aim_nx;
            by = in.aim_ny;
            normalizeOrDefault(bx, by);
        } else {
            int dx = last_fire_dx_;
            int dy = last_fire_dy_;
            if (dx == 0 && dy == 0) {
                dx = 1;
            }
            bx = static_cast<float>(dx);
            by = static_cast<float>(dy);
            normalizeOrDefault(bx, by);
        }
        const float cx = x_;
        const float cy = y_ - player_shot::kFeetToCenterWorld;
        const float ox = cx + bx * player_shot::kMuzzleOffsetWorld;
        const float oy = cy + by * player_shot::kMuzzleOffsetWorld;
        const double mult = static_cast<double>(world.playerOutgoingDamageMultiplier());
        const int out_dmg =
            std::max(1, static_cast<int>(std::lround(static_cast<double>(damage_) * mult)));
        world.spawnPlayerBullet(ox, oy, bx * player_shot::kBulletSpeed, by * player_shot::kBulletSpeed, out_dmg,
                                playerBulletVisualForCharacter(character_id_));
        fire_cool_ = fire_period_;
    }

    for (double& cd : skill_slot_cd_) {
        cd = std::max(0.0, cd - dt);
    }
    if (skill_anim_rem_ > 0.0) {
        skill_anim_rem_ = std::max(0.0, skill_anim_rem_ - dt);
        if (skill_anim_rem_ <= 0.0) {
            skill_anim_total_ = 0.0;
        }
    }

    if (in.skill_q && skill_slot_cd_[0] <= 0.0) {
        const ISkill& sk = ringBurstSkill();
        if (mp_ >= sk.mpCost()) {
            mp_ -= sk.mpCost();
            skill_slot_cd_[0] += sk.cooldownSeconds();
            const double sk_mult = static_cast<double>(world.playerOutgoingDamageMultiplier());
            const int out_skill_dmg =
                std::max(1, static_cast<int>(std::lround(static_cast<double>(damage_) * sk_mult)));
            SkillCastContext ctx{world, SkillCasterKind::Player, this, nullptr, x_, y_, out_skill_dmg, 0};
            sk.execute(ctx);
            constexpr double kSkillAnimFps = 12.0;
            constexpr int kSkillAnimFrames = 3;
            skill_anim_total_ = static_cast<double>(kSkillAnimFrames) / kSkillAnimFps;
            skill_anim_rem_ = skill_anim_total_;
        }
    }

    if (in.skill_e && skill_slot_cd_[1] <= 0.0) {
        const ISkill& sk = playerNarrowFanSkill();
        if (mp_ >= sk.mpCost()) {
            float ax = 0.f;
            float ay = 0.f;
            if (in.use_mouse_aim) {
                ax = in.aim_nx;
                ay = in.aim_ny;
                normalizeOrDefault(ax, ay);
            } else {
                int dx = last_fire_dx_;
                int dy = last_fire_dy_;
                if (dx == 0 && dy == 0) {
                    dx = 1;
                }
                ax = static_cast<float>(dx);
                ay = static_cast<float>(dy);
                normalizeOrDefault(ax, ay);
            }
            mp_ -= sk.mpCost();
            skill_slot_cd_[1] += sk.cooldownSeconds();
            const double sk_mult = static_cast<double>(world.playerOutgoingDamageMultiplier());
            const int out_skill_dmg =
                std::max(1, static_cast<int>(std::lround(static_cast<double>(damage_) * sk_mult)));
            SkillCastContext ctx{world, SkillCasterKind::Player, this, nullptr, x_, y_, out_skill_dmg, 0};
            ctx.player_skill_aim_nx = ax;
            ctx.player_skill_aim_ny = ay;
            sk.execute(ctx);
            constexpr double kSkillAnimFps = 12.0;
            constexpr int kSkillAnimFrames = 3;
            skill_anim_total_ = static_cast<double>(kSkillAnimFrames) / kSkillAnimFps;
            skill_anim_rem_ = skill_anim_total_;
        }
    }
}

bool EnemyActor::destroyed() const noexcept {
    return hp_ <= 0;
}

void EnemyActor::configureForArchetype(EnemyArchetype a) {
    archetype_ = a;
    behavior_ = makeEnemyBehavior(a);
    enemy_fire_cool_ = 0.0;
    incoming_damage_multiplier_ = 1.f;
    switch (a) {
    case EnemyArchetype::Melee:
        max_hp_ = hp_ = wave_combat::kMobMeleeHp;
        enemy_fire_period_ = wave_combat::kMobMeleeFirePeriod;
        break;
    case EnemyArchetype::Ranged:
        max_hp_ = hp_ = wave_combat::kMobRangedHp;
        enemy_fire_period_ = wave_combat::kMobRangedFirePeriod;
        break;
    case EnemyArchetype::EliteHybrid:
        max_hp_ = hp_ = wave_combat::kMobEliteHp;
        enemy_fire_period_ = wave_combat::kMobEliteFirePeriod;
        break;
    case EnemyArchetype::Boss:
        max_hp_ = hp_ = wave_combat::kMobBossHp;
        enemy_fire_period_ = wave_combat::kMobBossFirePeriod;
        boss_hurt_anim_rem_ = 0.0;
        boss_cast_anim_rem_ = 0.0;
        break;
    default:
        max_hp_ = hp_ = wave_combat::kMobEliteHp;
        enemy_fire_period_ = wave_combat::kMobEliteFirePeriod;
        break;
    }
}

void EnemyActor::resetForSpawn(EnemyArchetype a, EnemySpriteId id) {
    sprite_id_ = id;
    configureForArchetype(a);
}

void EnemyActor::applyDamage(int amount, World* world) {
    if (hp_ <= 0) {
        return;
    }
    const int scaled =
        std::max(1, static_cast<int>(std::lround(static_cast<double>(amount) * static_cast<double>(incoming_damage_multiplier_))));
    hp_ -= scaled;
    if (hp_ < 0) {
        hp_ = 0;
    }
    if (world != nullptr && archetype_ == EnemyArchetype::Boss && scaled > 0) {
        world->onBossDamagedByPlayer(scaled);
    }
    if (archetype_ == EnemyArchetype::Boss && scaled > 0) {
        armBossHurtFlash();
    }
    if (hp_ == 0 && world != nullptr) {
        world->notifyEnemyKilled(*this);
    }
}

void EnemyActor::tickBossPresentationTimers(double dt) {
    boss_hurt_anim_rem_ = std::max(0.0, boss_hurt_anim_rem_ - dt);
    boss_cast_anim_rem_ = std::max(0.0, boss_cast_anim_rem_ - dt);
}

void EnemyActor::armBossHurtFlash() {
    constexpr double kSec = 0.22;
    boss_hurt_anim_rem_ = std::max(boss_hurt_anim_rem_, kSec);
}

void EnemyActor::armBossCastVisual(double seconds) {
    if (seconds > 0.0) {
        boss_cast_anim_rem_ = std::max(boss_cast_anim_rem_, seconds);
    }
}

void EnemyActor::integrateVelocity(World& world, float mx, float my, float dt) {
    last_anim_vx_ = mx;
    last_anim_vy_ = my;
    const float nx = x_ + mx * dt;
    const float ny = y_ + my * dt;
    if (world.enemyFitsAt(nx, ny, this)) {
        x_ = nx;
        y_ = ny;
    } else if (world.enemyFitsAt(nx, y_, this)) {
        x_ = nx;
    } else if (world.enemyFitsAt(x_, ny, this)) {
        y_ = ny;
    }
}

void EnemyActor::step(World& world, double dt) {
    if (!behavior_) {
        configureForArchetype(archetype_);
    }
    CombatActorPorts ports;
    ports.fire = static_cast<IBulletFirePort*>(&world);
    ports.melee = static_cast<IMeleeEngagePort*>(&world);
    behavior_->tick(*this, ports, world, dt);
    tickBossPresentationTimers(dt);
}

bool BulletActor::destroyed() const noexcept {
    return dead_;
}

void BulletActor::integratePosition(World&, float dt_sec) {
    x_ += vx_ * dt_sec;
    y_ += vy_ * dt_sec;
}

void BulletActor::resolveBulletWorldAndHits(World& world) {
    if (dead_) {
        return;
    }
    const int gx = static_cast<int>(std::floor(x_));
    const int gy = static_cast<int>(std::floor(y_));
    if (!world.playfield().inBounds(gx, gy)) {
        dead_ = true;
        return;
    }

    if (!world.terrainCircleWalkable(x_, y_, kBulletBodyRadius)) {
        dead_ = true;
        return;
    }

    if (hit_policy_->resolveActorHits(*this, world)) {
        dead_ = true;
    }
}

void BulletActor::step(World& world, double dt) {
    if (dead_) {
        return;
    }
    const float fdt = static_cast<float>(dt);
    integratePosition(world, fdt);
    resolveBulletWorldAndHits(world);
}

void EnemyBulletActor::integratePosition(World& world, float fdt) {
    if (!soft_homing_) {
        BulletActor::integratePosition(world, fdt);
        checkMaxTravelAfterStep();
        return;
    }
    if (straight_rem_ > 0.0) {
        straight_rem_ -= static_cast<double>(fdt);
        BulletActor::integratePosition(world, fdt);
        checkMaxTravelAfterStep();
        return;
    }

    const float px = world.player().x();
    const float py = world.player().y();
    float tx = px - x_;
    float ty = py - y_;
    normalizeOrDefault(tx, ty);

    const float cur = std::atan2(vy_, vx_);
    const float tgt = std::atan2(ty, tx);
    float diff = tgt - cur;
    constexpr float kPi = 3.14159265358979323846f;
    while (diff > kPi) {
        diff -= 2.f * kPi;
    }
    while (diff < -kPi) {
        diff += 2.f * kPi;
    }
    const float max_step = max_turn_rad_per_sec_ * fdt;
    const float step = std::clamp(diff, -max_step, max_step);
    const float newa = cur + step;
    vx_ = std::cos(newa) * speed_scalar_;
    vy_ = std::sin(newa) * speed_scalar_;

    BulletActor::integratePosition(world, fdt);
    checkMaxTravelAfterStep();
}

} // namespace domain
