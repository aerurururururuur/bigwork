#pragma once

#include "domain/combat_entities.hpp"
#include "domain/combat_events.hpp"
#include "domain/combat_ports.hpp"
#include "domain/enemy_archetype.hpp"
#include "domain/pickup_drop.hpp"
#include "domain/playfield.hpp"
#include "domain/ports/irandom.hpp"
#include "domain/score_state.hpp"
#include "domain/wave_combat_tuning.hpp"

#include <memory>
#include <vector>

namespace domain {

enum class BattleOutcome { None, Victory, Defeat };

/** Score thresholds for player outgoing damage (from INI via Application). */
struct PlayerDamageScoreBrackets {
    int tier1_score{100};
    float tier1_mult{1.2f};
    int tier2_score{300};
    float tier2_mult{1.5f};
};

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
    const std::vector<PickupDrop>& pickups() const { return pickups_; }

    BattleOutcome battleOutcome() const { return outcome_; }

    /** Mob wave label for HUD (1..total); during intermission shows the upcoming wave. */
    int battleHudMobWave() const;
    int mobWavesTotal() const;
    double waveIntermissionRemaining() const { return wave_intermission_rem_; }
    int enemiesAliveCount() const;

    IRandom& random() { return rng_; }

    /** Intent visible to PlayerActor::step for this tick only. */
    const PlayerIntent& frameIntent() const { return frame_intent_; }

    void spawnPlayerBullet(float x, float y, float vx, float vy, int damage,
                           std::uint8_t player_bullet_visual = 0) override;

    void cyclePlayerCharacter();
    void setPlayerCharacter(PlayerCharacterId id);
    void spawnEnemyBullet(float x, float y, float vx, float vy, int damage,
                          EnemyBulletSprite sprite = EnemyBulletSprite::Generic,
                          std::uint8_t boss_bullet_strip = 0, float max_travel_sq = -1.f) override;

    /** Enemy bullet with optional soft homing after `straight_sec` (see `EnemyBulletActor`). */
    void spawnEnemyBulletSoftHoming(float x, float y, float vx, float vy, int damage, EnemyBulletSprite sprite,
                                    double straight_sec, float max_turn_rad_per_sec,
                                    std::uint8_t boss_bullet_strip = 0);

    void chasePlayerStep(EnemyActor& self, World& world, double dt, float chase_speed) override;
    void wanderStep(EnemyActor& self, World& world, double dt, float wander_speed,
                    float manhattan_aggro_radius) override;

    bool lineOfSightClear(float ax, float ay, float bx, float by) const;

    void onEnemyBulletHitPlayer(int raw_damage);
    void notifyEnemyKilled(EnemyActor& enemy);

    std::vector<CombatVfxEvent> drainCombatVfxEvents();

    const ScoreState& score() const { return score_; }

    /** From total score and configured tiers (strictly greater than threshold). */
    float playerOutgoingDamageMultiplier() const;

    /** Called once at boot from `GameConfig` (not reset per battle). */
    void setPlayerDamageScoreBrackets(const PlayerDamageScoreBrackets& b);

    void onBossDamagedByPlayer(int scaled_hp_lost);

    /** Debug: instant-kill every enemy with hp > 0 (score/drops via normal `applyDamage`). */
    void devKillAllEnemies();

    bool terrainCircleWalkable(float cx, float cy, float r) const;

    bool playerFitsAt(float x, float y) const;
    bool enemyFitsAt(float x, float y, const EnemyActor* self) const;

private:
    void scatterObstacles();
    void spawnEnemiesForWave(int wave_number);
    void maybeSpawnBossAfterWaveClear(double dt);
    void cullDynamics();
    void evaluateBattleOutcome();
    void applyContactDamage(double dt);

    void maybeSpawnPickupOnKill(const EnemyActor& enemy);
    void updatePickupsAndCollect(double dt);
    bool tryPlacePickup(PickupKind kind, float base_x, float base_y);

    void pushCombatVfx(CombatVfxKind kind, float wx, float wy);

    bool overlapsEnemiesCircle(float cx, float cy, float r, const EnemyActor* except) const;
    bool overlapsPlayerCircle(float cx, float cy, float r) const;

    IRandom& rng_;
    PlayfieldGrid playfield_;
    PlayerActor player_;
    std::vector<std::unique_ptr<EnemyActor>> enemies_;
    std::vector<std::unique_ptr<BulletActor>> bullets_;
    std::vector<PickupDrop> pickups_;
    PlayerIntent intent_{};
    PlayerIntent frame_intent_{};
    BattleOutcome outcome_{BattleOutcome::None};
    double contact_hurt_cool_{0.0};
    ScoreState score_;
    PlayerDamageScoreBrackets player_damage_brackets_{};
    std::vector<CombatVfxEvent> vfx_pending_;
    /** After all mob waves cleared, a single boss is spawned; then empty field means victory. */
    bool boss_released_{false};
    int mob_wave_index_{1};
    double wave_intermission_rem_{0.0};
    float player_bullet_max_travel_world_{wave_combat::kPlayerMobBulletTravelWorld};
};

} // namespace domain
