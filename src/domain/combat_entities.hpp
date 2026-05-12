#pragma once

#include <cstdint>

namespace domain {

class World;

struct PlayerIntent {
    int move_dx{0};
    int move_dy{0};
    bool fire{false};
    /** If true, use `aim_nx`/`aim_ny` (unit) for new shots instead of last keyboard direction. */
    bool use_mouse_aim{false};
    float aim_nx{0.f};
    float aim_ny{0.f};
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

    int gx() const { return gx_; }
    int gy() const { return gy_; }
    int hp() const { return hp_; }
    int maxHp() const { return max_hp_; }
    int lastFireDirX() const { return last_fire_dx_; }
    int lastFireDirY() const { return last_fire_dy_; }

private:
    friend class World;

    int gx_{0};
    int gy_{0};
    int hp_{10};
    int max_hp_{10};
    int armor_{0};
    int damage_{1};
    double move_period_{0.12};
    double fire_period_{0.28};
    double move_cool_{0.0};
    double fire_cool_{0.0};
    int last_fire_dx_{1};
    int last_fire_dy_{0};
};

class EnemyActor final : public CombatEntity {
public:
    void step(World& world, double dt) override;
    bool destroyed() const noexcept override;

    int gx() const { return gx_; }
    int gy() const { return gy_; }
    int hp() const { return hp_; }
    std::uint8_t kind() const { return kind_; }
    void applyDamage(int amount);

private:
    friend class World;

    int gx_{0};
    int gy_{0};
    int hp_{3};
    std::uint8_t kind_{0};
    double move_period_{0.35};
    double move_cool_{0.0};
};

class BulletActor final : public CombatEntity {
public:
    void step(World& world, double dt) override;
    bool destroyed() const noexcept override;

    float x() const { return x_; }
    float y() const { return y_; }
    float vx() const { return vx_; }
    float vy() const { return vy_; }
    int damage() const { return damage_; }

private:
    friend class World;

    float x_{0.f};
    float y_{0.f};
    float vx_{0.f};
    float vy_{0.f};
    int damage_{1};
    bool dead_{false};
};

} // namespace domain
