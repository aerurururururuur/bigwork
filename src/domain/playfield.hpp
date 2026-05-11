#pragma once

#include "domain/screen_layout.hpp"

#include <vector>

namespace domain {

/** Walkable + symbol per cell for the bottom strip (width = kCols, height = playfieldRows). */
class PlayfieldGrid {
public:
    PlayfieldGrid();

    int width() const { return ScreenLayout::kCols; }
    int height() const { return ScreenLayout::playfieldRows(); }

    bool inBounds(int x, int y) const;
    bool walkable(int x, int y) const;
    char tile(int x, int y) const;

private:
    std::vector<char> tiles_;
    std::vector<bool> walkable_;
};

} // namespace domain
