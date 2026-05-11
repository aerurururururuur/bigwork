#include "domain/world.hpp"

namespace domain {

World::World() {
    const int w = playfield_.width();
    const int h = playfield_.height();
    player_gx_ = w / 4;
    player_gy_ = h / 2;
    npc_.gx = w / 2 + 6;
    npc_.gy = h / 2;
    scroll_world_ = static_cast<double>(player_gx_);
}

void World::tryMovePlayer(int dx, int dy) {
    const int nx = player_gx_ + dx;
    const int ny = player_gy_ + dy;
    if (!playfield_.walkable(nx, ny)) {
        return;
    }
    player_gx_ = nx;
    player_gy_ = ny;
    scroll_world_ += static_cast<double>(dx) * 0.15;
}

} // namespace domain
