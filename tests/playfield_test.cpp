#include "domain/playfield.hpp"

#include <gtest/gtest.h>

TEST(PlayfieldGrid, InteriorDefaultWalkable) {
    domain::PlayfieldGrid g;
    ASSERT_TRUE(g.inBounds(5, 5));
    EXPECT_TRUE(g.walkable(5, 5));
}

TEST(PlayfieldGrid, BorderNotWalkable) {
    domain::PlayfieldGrid g;
    EXPECT_FALSE(g.walkable(0, 5));
    EXPECT_FALSE(g.walkable(5, 0));
}

TEST(PlayfieldGrid, ObstacleThenClear) {
    domain::PlayfieldGrid g;
    const int x = 10;
    const int y = 10;
    ASSERT_TRUE(g.walkable(x, y));
    g.setKind(x, y, domain::TileKind::Obstacle);
    EXPECT_FALSE(g.walkable(x, y));
    g.clearObstaclesToFloor();
    EXPECT_TRUE(g.walkable(x, y));
}
