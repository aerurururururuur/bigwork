#pragma once

#include "domain/screen_layout.hpp"
#include "domain/tile_base.hpp"

#include <vector>

namespace domain {

/** Room grid: stores TileKind per cell; semantics from TileBase subclasses. */
class PlayfieldGrid {
public:
    PlayfieldGrid();

    int width() const { return ScreenLayout::kCols; }
    int height() const { return ScreenLayout::playfieldRows(); }

    bool inBounds(int x, int y) const;
    bool walkable(int x, int y) const;
    char tile(int x, int y) const;
    TileKind tileKind(int x, int y) const;

    void setKind(int x, int y, TileKind k);

private:
    std::vector<TileKind> kinds_;
};

} // namespace domain
