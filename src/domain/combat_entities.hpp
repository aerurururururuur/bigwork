#pragma once

#include "domain/bullet_faction.hpp"
#include "domain/enemy_archetype.hpp"
#include "domain/enemy_bullet_sprite.hpp"
#include "domain/enemy_sprite_id.hpp"

#include <array>
#include <cstdint>
#include <memory>

namespace domain {

class IBulletHitPolicy;
class IEnemyBehavior;
class World;

/** Body radius in grid units for enemies (terrain / each other / melee vs player bullet math baseline). */
inline constexpr float kActorBodyRadius = 0.35f;
/** Smaller player hitbox so the body fits through single-tile gaps more reliably. */
inline constexpr float kPlayerBodyRadius = 0.26f;
/**
 * Enemy radius used only for **player bullet** hit tests: larger than `kActorBodyRadius` so hits match
 * visible sprite overlap (Representation draws enemies bigger than this physics disc).
 */
inline constexpr float kEnemyPlayerBulletHitRadius = 0.54f;
/** Bullet collision radius in world units. */
inline constexpr float kBulletBodyRadius = 0.12f;

/** Shared tuning for player-fired bullets (basic shot and skills). */
namespace player_shot {
inline constexpr float kBulletSpeed = 9.0f;
inline constexpr float kFeetToCenterWorld = 0.38f;
inline constexpr float kMuzzleOffsetWorld = 0.06f;
} // namespace player_shot

struct PlayerIntent {
    int move_dx{0};
    int move_dy{0};
    bool fire{false};
    /** If true, use `aim_nx`/`aim_ny` (unit) for new shots instead of last keyboard direction. */
    bool use_mouse_aim{false};
    float aim_nx{0.f};
    float aim_ny{0.f};
    /** Edge-triggered skill (Q); consumed same frame by `PlayerActor::step`. */
    bool skill_q{false};
};

/** Dynamic entities (player, foes, shots) share this extension root. */
class CombatEntity {
public:
    virtual ~CombatEntity() = default;
    virtual void step(World& world, double dt) = 0;
    virtual bool destroyed() const noexcept = 0;
};

class PlayerActor final : public CombatEntity {
public:
    void step(World& world, double dt) override;
    bool destroyed() const noexcept override;

    float x() const { return x_; }
    float y() const { return y_; }
    float vx() const { return vx_; }
    float vy() const { return vy_; }
    int hp() const { return hp_; }
    int maxHp() const { return max_hp_; }
    int mp() const { return mp_; }
    int maxMp() const { return max_mp_; }
    int lastFireDirX() const { return last_fire_dx_; }
    int lastFireDirY() const { return last_fire_dy_; }
    double bulletInvulnRemaining() const { return bullet_invuln_rem_; }
    int shotDamage() const { return damage_; }

    double skillAnimRemaining() const { return skill_anim_rem_; }
    double skillAnimTotal() const { return skill_anim_total_; }

    void resetSkillState();

private:
    friend class World;

    float x_{0.f};
    float y_{0.f};
    float vx_{0.f};
    float vy_{0.f};
    int hp_{10};
    int max_hp_{10};
    int mp_{30};
    int max_mp_{30};
    double mp_regen_carry_{0.0};
    int armor_{0};
    int damage_{1};
    double fire_period_{0.28};
    double fire_cool_{0.0};
    int last_fire_dx_{1};
    int last_fire_dy_{0};
    /** Separate from melee contact i-frame (managed on World). */
    double bullet_invuln_rem_{0.0};

    static constexpr int kSkillSlotCount = 4;
    std::array<double, kSkillSlotCount> skill_slot_cd_{};
    double skill_anim_rem_{0.0};
    double skill_anim_total_{0.0};
};

class IEnemyBehavior;

class EnemyActor final : public CombatEntity {
public:
    ~EnemyActor() override;
    void step(World& world, double dt) override;
    bool destroyed() const noexcept override;

    float x() const { return x_; }
    float y() const { return y_; }
    int hp() const { return hp_; }
    int maxHp() const { return max_hp_; }
    EnemyArchetype archetype() const { return archetype_; }
    EnemySpriteId spriteId() const { return sprite_id_; }
    float lastAnimVx() const { return last_anim_vx_; }
    float lastAnimVy() const { return last_anim_vy_; }
    void applyDamage(int amount, World* world);

    /** Called when placing an enemy in the room (sets archetype + stats + AI). */
    void resetForSpawn(EnemyArchetype a, EnemySpriteId sprite_id);

    double enemyFireCoolRemaining() const { return enemy_fire_cool_; }
    void advanceEnemyFireCooldown(double dt) { enemy_fire_cool_ -= dt; }
    void armEnemyFireCooldown() { enemy_fire_cool_ = enemy_fire_period_; }

    /** Move with sliding collision (world units/sec velocity). */
    void integrateVelocity(World& world, float mx, float my, float dt);

private:
    friend class World;

    void configureForArchetype(EnemyArchetype a);

    float x_{0.f};
    float y_{0.f};
    int hp_{3};
    int max_hp_{3};
    EnemyArchetype archetype_{EnemyArchetype::Melee};
    EnemySpriteId sprite_id_{EnemySpriteId::Slime};
    float last_anim_vx_{0.f};
    float last_anim_vy_{0.f};
    double enemy_fire_cool_{0.0};
    double enemy_fire_period_{0.85};
    std::unique_ptr<IEnemyBehavior> behavior_;
};

/** Base bullet: movement, terrain, hit policy; subclasses only fix faction / policy wiring. */
class BulletActor : public CombatEntity {
public:
    ~BulletActor() override;
    void step(World& world, double dt) override;
    bool destroyed() const noexcept override;

    float x() const { return x_; }
    float y() const { return y_; }
    float vx() const { return vx_; }
    float vy() const { return vy_; }
    int damage() const { return damage_; }
    virtual BulletFaction faction() const noexcept = 0;
    /** Non-zero only for enemy bullets; used by Application snapshot (no RTTI). */
    virtual std::uint8_t enemyBulletVisual() const noexcept { return 0; }

protected:
    BulletActor(float x, float y, float vx, float vy, int damage, std::unique_ptr<IBulletHitPolicy> policy);

    float x_{0.f};
    float y_{0.f};
    float vx_{0.f};
    float vy_{0.f};
    int damage_{1};
    bool dead_{false};
    std::unique_ptr<IBulletHitPolicy> hit_policy_;
};

class PlayerBulletActor final : public BulletActor {
public:
    PlayerBulletActor(float x, float y, float vx, float vy, int damage);
    BulletFaction faction() const noexcept override { return BulletFaction::Player; }
};

class EnemyBulletActor final : public BulletActor {
public:
    EnemyBulletActor(float x, float y, float vx, float vy, int damage,
                       EnemyBulletSprite sprite = EnemyBulletSprite::Generic);
    BulletFaction faction() const noexcept override { return BulletFaction::Enemy; }
    std::uint8_t enemyBulletVisual() const noexcept override {
        return static_cast<std::uint8_t>(sprite_);
    }

private:
    EnemyBulletSprite sprite_{EnemyBulletSprite::Generic};
};

} // namespace domain
