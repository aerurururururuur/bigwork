#include "domain/playfield.hpp"

namespace domain {

PlayfieldGrid::PlayfieldGrid() {
    const int w = width();
    const int h = height();
    tiles_.assign(static_cast<std::size_t>(w * h), '.');
    walkable_.assign(static_cast<std::size_t>(w * h), true);

    const int mid = h / 2;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const std::size_t i = static_cast<std::size_t>(y * w + x);
            if (x == 0 || x == w - 1 || y == 0 || y == h - 1) {
                tiles_[i] = '#';
                walkable_[i] = false;
            } else if (y == mid) {
                tiles_[i] = '=';
            } else {
                tiles_[i] = ',';
            }
        }
    }
}

bool PlayfieldGrid::inBounds(int x, int y) const {
    return x >= 0 && x < width() && y >= 0 && y < height();
}

bool PlayfieldGrid::walkable(int x, int y) const {
    if (!inBounds(x, y)) {
        return false;
    }
    return walkable_[static_cast<std::size_t>(y * width() + x)];
}

char PlayfieldGrid::tile(int x, int y) const {
    if (!inBounds(x, y)) {
        return ' ';
    }
    return tiles_[static_cast<std::size_t>(y * width() + x)];
}

} // namespace domain
