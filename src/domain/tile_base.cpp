#include "domain/tile_base.hpp"

namespace domain {

namespace {

const FloorTile kFloor{};
const WallTile kWall{};
const ObstacleTile kObstacle{};

} // namespace

const TileBase& tilePrototype(TileKind kind) {
    switch (kind) {
    case TileKind::Wall:
        return kWall;
    case TileKind::Obstacle:
        return kObstacle;
    case TileKind::Floor:
    default:
        return kFloor;
    }
}

} // namespace domain
