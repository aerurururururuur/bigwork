#pragma once

namespace domain {

/** Full-screen logical room (each cell maps to one SFML tile). */
struct ScreenLayout {
    static constexpr int kRows = 27;
    static constexpr int kCols = 48;
    /** 1.0 => entire grid is the combat playfield (no sky strip). */
    static constexpr float kPlayfieldFraction = 1.0f;

    static constexpr int playfieldRows() {
        return static_cast<int>(static_cast<float>(kRows) * kPlayfieldFraction);
    }
    static constexpr int skyRows() { return kRows - playfieldRows(); }
};

} // namespace domain
