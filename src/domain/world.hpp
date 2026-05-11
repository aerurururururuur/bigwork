#pragma once

#include "domain/playfield.hpp"

namespace domain {

struct NpcState {
    int gx{0};
    int gy{0};
    char symbol{'N'};
};

class World {
public:
    World();

    PlayfieldGrid& playfield() { return playfield_; }
    const PlayfieldGrid& playfield() const { return playfield_; }

    int playerGx() const { return player_gx_; }
    int playerGy() const { return player_gy_; }
    double scrollWorld() const { return scroll_world_; }

    const NpcState& npc() const { return npc_; }

    /** Attempt move in playfield grid; updates scroll tie for parallax. */
    void tryMovePlayer(int dx, int dy);

private:
    PlayfieldGrid playfield_;
    int player_gx_{0};
    int player_gy_{0};
    double scroll_world_{0.0};
    NpcState npc_;
};

} // namespace domain
