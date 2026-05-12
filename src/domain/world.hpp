#pragma once

#include "domain/combat_entities.hpp"
#include "domain/combat_events.hpp"
#include "domain/combat_ports.hpp"
#include "domain/enemy_archetype.hpp"
#include "domain/playfield.hpp"
#include "domain/ports/irandom.hpp"
#include "domain/score_state.hpp"

#include <memory>
#include <vector>

namespace domain {

enum class BattleOutcome { None, Victory, Defeat };

class World final : public IBulletFirePort, public IMeleeEngagePort {
    friend class PlayerActor;
    friend class EnemyActor;

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

    void spawnPlayerBullet(float x, float y, float vx, float vy, int damage) override;
    void spawnEnemyBullet(float x, float y, float vx, float vy, int damage,
                          EnemyBulletSprite sprite = EnemyBulletSprite::Generic) override;

    void chasePlayerStep(EnemyActor& self, World& world, double dt, float chase_speed) override;
    void wanderStep(EnemyActor& self, World& world, double dt, float wander_speed,
                    float manhattan_aggro_radius) override;

    bool lineOfSightClear(float ax, float ay, float bx, float by) const;

    void onEnemyBulletHitPlayer(int raw_damage);
    void notifyEnemyKilled(EnemyActor& enemy);

    std::vector<CombatVfxEvent> drainCombatVfxEvents();

    const ScoreState& score() const { return score_; }

    bool terrainCircleWalkable(float cx, float cy, float r) const;

    bool playerFitsAt(float x, float y) const;
    bool enemyFitsAt(float x, float y, const EnemyActor* self) const;

private:
    void scatterObstacles();
    void spawnEnemies();
    void cullDynamics();
    void evaluateBattleOutcome();
    void applyContactDamage(double dt);

    void pushCombatVfx(CombatVfxKind kind, float wx, float wy);

    bool overlapsEnemiesCircle(float cx, float cy, float r, const EnemyActor* except) const;
    bool overlapsPlayerCircle(float cx, float cy, float r) const;

    IRandom& rng_;
    PlayfieldGrid playfield_;
    PlayerActor player_;
    std::vector<std::unique_ptr<EnemyActor>> enemies_;
    std::vector<std::unique_ptr<BulletActor>> bullets_;
    PlayerIntent intent_{};
    PlayerIntent frame_intent_{};
    BattleOutcome outcome_{BattleOutcome::None};
    double contact_hurt_cool_{0.0};
    ScoreState score_;
    std::vector<CombatVfxEvent> vfx_pending_;
};

} // namespace domain
