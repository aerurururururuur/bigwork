#include "domain/score_state.hpp"

#include <algorithm>

namespace domain {

namespace {

constexpr double kComboWindowSec = 2.2;
constexpr int kComboMultiplierCap = 8;

} // namespace

int TableScoreRule::basePoints(EnemyArchetype t) {
    switch (t) {
    case EnemyArchetype::Melee:
        return 10;
    case EnemyArchetype::Ranged:
        return 18;
    case EnemyArchetype::EliteHybrid:
        return 35;
    case EnemyArchetype::Boss:
        return 80;
    default:
        return 10;
    }
}

void TableScoreRule::onEnemyKilled(ScoreState& s, EnemyArchetype t) const {
    const int base = basePoints(t);
    const int mult = std::max(1, std::min(kComboMultiplierCap, 1 + s.combo_));
    s.total_score_ += base * mult;
    s.combo_ = std::min(kComboMultiplierCap, s.combo_ + 1);
    s.combo_timer_ = kComboWindowSec;
}

ScoreState::ScoreState() : ScoreState(std::make_unique<TableScoreRule>()) {}

void ScoreState::reset() {
    total_score_ = 0;
    combo_ = 0;
    combo_timer_ = 0.0;
}

void TableScoreRule::tickDecay(ScoreState& s, double dt) const {
    if (s.combo_ <= 0) {
        return;
    }
    s.combo_timer_ -= dt;
    if (s.combo_timer_ <= 0.0) {
        s.combo_ = 0;
        s.combo_timer_ = 0.0;
    }
}

ScoreState::ScoreState(std::unique_ptr<IScoreRule> rule) : rule_(std::move(rule)) {
    if (!rule_) {
        rule_ = std::make_unique<TableScoreRule>();
    }
}

void ScoreState::onEnemyKilled(EnemyArchetype t) {
    rule_->onEnemyKilled(*this, t);
}

void ScoreState::tick(double dt) {
    rule_->tickDecay(*this, dt);
}

} // namespace domain
