#pragma once

namespace domain {

/** Logical console grid; 16:9 style composition (character cells, not square pixels). */
struct ScreenLayout {
    static constexpr int kRows = 27;
    static constexpr int kCols = 48;
    static constexpr float kPlayfieldFraction = 0.25f;

    static constexpr int playfieldRows() { return kRows / 4; }
    static constexpr int skyRows() { return kRows - playfieldRows(); }
};

} // namespace domain
