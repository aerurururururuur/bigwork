#include "representation/frame_composer.hpp"

#include <algorithm>
#include <string>

namespace {

constexpr const char* kSkyLayer1 = "~^`'..::";
constexpr const char* kSkyLayer2 = "#*+=-";

char pickSkyChar(int row, int col, int scroll1, int scroll2) {
    const int i1 = (col + scroll1 + row * 3) % 8;
    const int i2 = (col / 2 + scroll2 + row) % 6;
    if (row % 3 == 0) {
        return kSkyLayer2[(i2 < 0 ? -i2 : i2) % 6];
    }
    return kSkyLayer1[(i1 < 0 ? -i1 : i1) % 8];
}

std::uint8_t skyColor(application::ThemeStyle t, int row) {
    if (t == application::ThemeStyle::ColdNight) {
        return static_cast<std::uint8_t>(10 + (row % 3));
    }
    return static_cast<std::uint8_t>(20 + (row % 4));
}

std::uint8_t playfieldColor(application::ThemeStyle t, char ch) {
    if (ch == '#') {
        return 2;
    }
    if (ch == '=') {
        return t == application::ThemeStyle::Dusk ? 3 : 13;
    }
    return t == application::ThemeStyle::Dusk ? 4 : 14;
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
            if (r < snap.sky_rows) {
                const char ch = pickSkyChar(r, c, snap.sky_scroll_slow, snap.sky_scroll_fast);
                cell.ch = ch;
                cell.color_id = skyColor(snap.theme, r);
            } else {
                const int py = r - snap.sky_rows;
                const std::size_t idx = static_cast<std::size_t>(py * C + c);
                char tch = '.';
                if (py >= 0 && py < snap.playfield_rows && c >= 0 && c < C &&
                    idx < snap.playfield_tiles.size()) {
                    tch = snap.playfield_tiles[idx];
                }
                cell.ch = tch;
                cell.color_id = playfieldColor(snap.theme, tch);

                if (c == snap.player_x && py == snap.player_y) {
                    cell.ch = '@';
                    cell.color_id = 5;
                } else if (c == snap.npc_x && py == snap.npc_y) {
                    cell.ch = 'N';
                    cell.color_id = 6;
                }
            }
            out[static_cast<std::size_t>(r * C + c)] = cell;
        }
    }

    if (snap.overlay.active && !snap.overlay.lines.empty()) {
        std::size_t max_len = 0;
        for (const std::string& ln : snap.overlay.lines) {
            max_len = std::max(max_len, ln.size());
        }
        const int box_w = static_cast<int>(max_len) + 4;
        const int box_h = static_cast<int>(snap.overlay.lines.size()) + 2;
        const int ox = std::max(0, (C - box_w) / 2);
        const int oy = std::max(0, (R - box_h) / 2);
        for (int y = 0; y < box_h; ++y) {
            for (int x = 0; x < box_w; ++x) {
                const int rr = oy + y;
                const int cc = ox + x;
                if (rr < 0 || rr >= R || cc < 0 || cc >= C) {
                    continue;
                }
                char ch = ' ';
                if (y == 0 || y == box_h - 1) {
                    ch = (x == 0 || x == box_w - 1) ? '+' : '-';
                } else if (x == 0 || x == box_w - 1) {
                    ch = '|';
                } else {
                    const int line = y - 1;
                    const int col = x - 1;
                    if (line >= 0 && line < static_cast<int>(snap.overlay.lines.size())) {
                        const std::string& ln = snap.overlay.lines[static_cast<std::size_t>(line)];
                        if (col >= 0 && col < static_cast<int>(ln.size())) {
                            ch = ln[static_cast<std::size_t>(col)];
                        }
                    }
                }
                domain::FrameCell& cell = out[static_cast<std::size_t>(rr * C + cc)];
                cell.ch = ch;
                cell.color_id = 7;
            }
        }
    }

    return out;
}

} // namespace representation
