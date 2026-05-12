#pragma once

#include "domain/combat_entities.hpp"
#include "domain/playfield.hpp"
#include "domain/ports/irandom.hpp"

#include <memory>
#include <vector>

namespace domain {

enum class BattleOutcome { None, Victory, Defeat };

class World {
    friend class PlayerActor;
    friend class EnemyActor;
    friend class BulletActor;

public:
    explicit World(IRandom& rng);

    void setIntent(PlayerIntent in) { intent_ = in; }
    void simulateStep(double dt);
    void resetBattle();

    PlayfieldGrid& playfield() { return playfield_; }
    const PlayfieldGrid& playfield() const { return playfield_; }

    const PlayerActor& player() const { return player_; }
    const std::vector<std::unique_ptr<EnemyActor>>& enemies() const { return enemies_; }
    const std::vector<std::unique_ptr<BulletActor>>& bullets() const { return bullets_; }

    BattleOutcome battleOutcome() const { return outcome_; }

    IRandom& random() { return rng_; }

    /** Intent visible to PlayerActor::step for this tick only. */
    const PlayerIntent& frameIntent() const { return frame_intent_; }

    void spawnBullet(float x, float y, float vx, float vy, int damage);

private:
    void scatterObstacles();
    void spawnEnemies();
    void cullDynamics();
    void evaluateBattleOutcome();
    void applyContactDamage(double dt);
    bool enemyAt(int gx, int gy) const;
    bool walkableNoActor(int gx, int gy) const;

    IRandom& rng_;
    PlayfieldGrid playfield_;
    PlayerActor player_;
    std::vector<std::unique_ptr<EnemyActor>> enemies_;
    std::vector<std::unique_ptr<BulletActor>> bullets_;
    PlayerIntent intent_{};
    PlayerIntent frame_intent_{};
    BattleOutcome outcome_{BattleOutcome::None};
    double contact_hurt_cool_{0.0};
};

} // namespace domain
