#include "domain/playfield.hpp"

namespace domain {

PlayfieldGrid::PlayfieldGrid() {
    const int w = width();
    const int h = height();
    kinds_.assign(static_cast<std::size_t>(w * h), TileKind::Floor);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (x == 0 || x == w - 1 || y == 0 || y == h - 1) {
                setKind(x, y, TileKind::Wall);
            }
        }
    }
}

bool PlayfieldGrid::inBounds(int x, int y) const {
    return x >= 0 && x < width() && y >= 0 && y < height();
}

TileKind PlayfieldGrid::tileKind(int x, int y) const {
    if (!inBounds(x, y)) {
        return TileKind::Wall;
    }
    return kinds_[static_cast<std::size_t>(y * width() + x)];
}

bool PlayfieldGrid::walkable(int x, int y) const {
    if (!inBounds(x, y)) {
        return false;
    }
    return tilePrototype(tileKind(x, y)).walkable();
}

char PlayfieldGrid::tile(int x, int y) const {
    if (!inBounds(x, y)) {
        return ' ';
    }
    return tilePrototype(tileKind(x, y)).glyph();
}

void PlayfieldGrid::setKind(int x, int y, TileKind k) {
    if (!inBounds(x, y)) {
        return;
    }
    kinds_[static_cast<std::size_t>(y * width() + x)] = k;
}

} // namespace domain
