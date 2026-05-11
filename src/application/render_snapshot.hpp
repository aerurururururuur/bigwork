#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace application {

enum class ThemeStyle : std::uint8_t { Dusk = 0, ColdNight = 1 };

struct OverlayModel {
    bool active{false};
    std::vector<std::string> lines;
};

/** Read-only view data for one frame (Representation consumes only this). */
struct RenderSnapshot {
    int total_rows{0};
    int total_cols{0};
    int sky_rows{0};
    int playfield_rows{0};
    int sky_scroll_slow{0};
    int sky_scroll_fast{0};
    std::vector<char> playfield_tiles;
    int player_x{0};
    int player_y{0};
    int npc_x{0};
    int npc_y{0};
    ThemeStyle theme{ThemeStyle::Dusk};
    OverlayModel overlay;
};

} // namespace application
