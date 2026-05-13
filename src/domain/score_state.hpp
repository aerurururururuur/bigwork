#pragma once

#include "domain/enemy_archetype.hpp"

#include <memory>

namespace domain {

class ScoreState;

/** Table-driven scoring + combo decay; replace for events / double-score modes. */
class IScoreRule {
public:
    virtual ~IScoreRule() = default;
    virtual void onEnemyKilled(ScoreState& s, EnemyArchetype t) const = 0;
    virtual void tickDecay(ScoreState& s, double dt) const = 0;
};

class ScoreState {
public:
    ScoreState();
    explicit ScoreState(std::unique_ptr<IScoreRule> rule);

    void reset();

    int totalScore() const { return total_score_; }
    int combo() const { return combo_; }
    double comboTimer() const { return combo_timer_; }

    void onEnemyKilled(EnemyArchetype t);
    void tick(double dt);

    /** Boss only: add score from HP chipped by player bullets (no combo change). */
    void addBossChipScore(int scaled_hp_lost);

private:
    friend class IScoreRule;
    friend class TableScoreRule;

    int total_score_{0};
    int combo_{0};
    double combo_timer_{0.0};
    std::unique_ptr<IScoreRule> rule_;
};

/** Default: per-archetype base points, combo stacks, timer decays combo. */
class TableScoreRule final : public IScoreRule {
public:
    void onEnemyKilled(ScoreState& s, EnemyArchetype t) const override;
    void tickDecay(ScoreState& s, double dt) const override;

private:
    static int basePoints(EnemyArchetype t);
};

} // namespace domain
