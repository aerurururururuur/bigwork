#include "representation/frame_composer.hpp"

#include <cstdint>

namespace {

std::uint8_t tileColor(application::ThemeStyle t, char ch) {
    switch (ch) {
    case '#':
        return 41;
    case 'O':
        return 42;
    case '.':
    default:
        return t == application::ThemeStyle::Dusk ? 40 : 48;
    }
}

} // namespace

namespace representation {

std::vector<domain::FrameCell> FrameComposer::compose(const application::RenderSnapshot& snap) const {
    const int R = snap.total_rows;
    const int C = snap.total_cols;
    std::vector<domain::FrameCell> out(static_cast<std::size_t>(R * C));

    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            domain::FrameCell cell{};
            const int py = r - snap.sky_rows;
            char tch = '.';
            if (py >= 0 && py < snap.playfield_rows && c >= 0 && c < C) {
                const std::size_t idx = static_cast<std::size_t>(py * C + c);
                if (idx < snap.playfield_tiles.size()) {
                    tch = snap.playfield_tiles[idx];
                }
            }
            cell.ch = tch;
            cell.color_id = tileColor(snap.theme, tch);

            out[static_cast<std::size_t>(r * C + c)] = cell;
        }
    }

    return out;
}

} // namespace representation
