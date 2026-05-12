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
    int bullet_damage{1};
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

} // namespace domain
