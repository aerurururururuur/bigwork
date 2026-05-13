#include "domain/enemy_archetype.hpp"
#include "domain/score_state.hpp"

#include <gtest/gtest.h>

TEST(ScoreState, FirstMeleeKillUsesComboOne) {
    domain::ScoreState s;
    s.onEnemyKilled(domain::EnemyArchetype::Melee);
    EXPECT_EQ(s.totalScore(), 10);
    EXPECT_EQ(s.combo(), 1);
}

TEST(ScoreState, ComboMultipliesSecondKill) {
    domain::ScoreState s;
    s.onEnemyKilled(domain::EnemyArchetype::Melee);
    s.onEnemyKilled(domain::EnemyArchetype::Melee);
    EXPECT_EQ(s.totalScore(), 10 + 20);
    EXPECT_EQ(s.combo(), 2);
}

TEST(ScoreState, ComboDecaysAfterWindow) {
    domain::ScoreState s;
    s.onEnemyKilled(domain::EnemyArchetype::Melee);
    s.tick(3.0);
    EXPECT_EQ(s.combo(), 0);
}

TEST(ScoreState, BossMinionBaseScore) {
    domain::ScoreState s;
    s.onEnemyKilled(domain::EnemyArchetype::BossMinion);
    EXPECT_EQ(s.totalScore(), 8);
}

TEST(ScoreState, BossChipDoesNotAdvanceCombo) {
    domain::ScoreState s;
    s.onEnemyKilled(domain::EnemyArchetype::Melee);
    s.addBossChipScore(5);
    EXPECT_EQ(s.totalScore(), 10 + 5);
    EXPECT_EQ(s.combo(), 1);
}

TEST(ScoreState, ComboMultiplierCapsAtEight) {
    domain::ScoreState s;
    for (int i = 0; i < 10; ++i) {
        s.onEnemyKilled(domain::EnemyArchetype::Melee);
    }
    EXPECT_EQ(s.combo(), 8);
    EXPECT_EQ(s.totalScore(), 520);
}
