#include "domain/score_state.hpp"

#include <algorithm>

/*
 * Scoring (TableScoreRule):
 * - On enemy kill: gain basePoints(archetype) * comboMultiplier, then combo++ and combo_timer reset.
 *   Base: Melee 10, Ranged 18, EliteHybrid 35, Boss 80. comboMultiplier = clamp(1 + combo_before_kill, 1..8).
 *   Combo window: kComboWindowSec (2.2s); when timer expires, combo resets to 0.
 * - Boss chip damage (ScoreState::addBossChipScore): each point of actual HP lost to player bullets on the
 *   Boss adds 1 to total_score only (no combo change). See World::onBossDamagedByPlayer.
 * - Player outgoing damage vs total score: thresholds in game_config.ini (player_damage_score_tier1/2, mults);
 *   computed in World::playerOutgoingDamageMultiplier (strictly greater than threshold uses higher tier).
 */

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
    case EnemyArchetype::BossMinion:
        return 8;
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

void ScoreState::addBossChipScore(int scaled_hp_lost) {
    if (scaled_hp_lost <= 0) {
        return;
    }
    total_score_ += scaled_hp_lost;
}

} // namespace domain
