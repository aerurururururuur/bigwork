#pragma once

#include <cstdint>

namespace domain {

/** Terrain cell; subclass to add new floor / wall / obstacle kinds. */
class TileBase {
public:
    virtual ~TileBase() = default;
    virtual bool walkable() const noexcept = 0;
    virtual char glyph() const noexcept = 0;
};

class FloorTile final : public TileBase {
public:
    bool walkable() const noexcept override { return true; }
    char glyph() const noexcept override { return '.'; }
};

class WallTile final : public TileBase {
public:
    bool walkable() const noexcept override { return false; }
    char glyph() const noexcept override { return '#'; }
};

class ObstacleTile final : public TileBase {
public:
    bool walkable() const noexcept override { return false; }
    char glyph() const noexcept override { return 'O'; }
};

enum class TileKind : std::uint8_t { Floor = 0, Wall = 1, Obstacle = 2 };

const TileBase& tilePrototype(TileKind kind);

} // namespace domain
