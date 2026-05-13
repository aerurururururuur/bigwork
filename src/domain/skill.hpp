#pragma once

namespace domain {

class World;
class PlayerActor;
class EnemyActor;

enum class SkillCasterKind { Player, Boss };

/** Context for executing a skill (player or boss). */
struct SkillCastContext {
    World& world;
    SkillCasterKind caster{SkillCasterKind::Player};
    PlayerActor* player{nullptr};
    EnemyActor* enemy{nullptr};
    /** Player feet position in world space (same as `PlayerActor::x/y`). */
    float player_foot_x{0.f};
    float player_foot_y{0.f};
    /** Player ring burst damage (`RingBurstSkill`). */
    int bullet_damage{1};
    /** Boss ring burst damage (`BossRingBurstSkill`). */
    int enemy_bullet_damage{1};
    /** Unit aim for player narrow-fan skill (world X right, Y down). */
    float player_skill_aim_nx{1.f};
    float player_skill_aim_ny{0.f};
};

/** Base skill: slot logic (MP / cooldown) is enforced by the caster; `execute` applies the effect. */
class ISkill {
public:
    virtual ~ISkill() = default;
    virtual int mpCost() const = 0;
    virtual double cooldownSeconds() const = 0;
    virtual void execute(SkillCastContext& ctx) const = 0;
};

/** Q-slot test skill: ring of player bullets, 10 MP, no cooldown. */
class RingBurstSkill final : public ISkill {
public:
    int mpCost() const override;
    double cooldownSeconds() const override;
    void execute(SkillCastContext& ctx) const override;
};

/** Singleton for the registered ring burst definition. */
const ISkill& ringBurstSkill();

/** E-slot: narrow fan of player bullets toward `player_skill_aim_*` (mouse or last move). */
class NarrowFanSkill final : public ISkill {
public:
    int mpCost() const override;
    double cooldownSeconds() const override;
    void execute(SkillCastContext& ctx) const override;
};

const ISkill& playerNarrowFanSkill();

/** Boss timed skill: ring of enemy bullets (no MP). */
class BossRingBurstSkill final : public ISkill {
public:
    int mpCost() const override;
    double cooldownSeconds() const override;
    void execute(SkillCastContext& ctx) const override;
};

const ISkill& bossRingBurstSkill();

/** Boss ring diffusion: first ring 12 shots; AI fires ring2 after 0.5s. */
void bossDiffusionFireRing1(SkillCastContext& ctx);
/** Second ring: 18 shots, global angle offset pi/12 vs first ring. */
void bossDiffusionFireRing2(SkillCastContext& ctx);

/** Fan aimed at player snapshot: +-30 deg, 20-28 rock bullets, medium speed. */
void bossFanBarrageFire(SkillCastContext& ctx);

/** One-shot spiral snapshot (tangent bullets, expanding radius). */
void bossSpiralBurstFire(SkillCastContext& ctx);
/** Left/right opposing fans perpendicular to aim. */
void bossDualOpposingFanFire(SkillCastContext& ctx);
/** Outer ring outward + inner ring inward. */
void bossCrossDualRingFire(SkillCastContext& ctx);
/** 8-10 soft-homing rocks: straight then gentle turn toward player. */
void bossSoftScatterFire(SkillCastContext& ctx);

/** Horizontal volley from left or right playfield wall (non-homing). */
void bossSideWallVolleyFire(SkillCastContext& ctx);

/**
 * Number of boss patterns bound to development hotkeys 1..N (max key index 9).
 * Increase when adding new `boss*` fire functions and manual dispatch.
 */
inline constexpr int kBossManualSkillHotkeyCount = 8;

} // namespace domain
