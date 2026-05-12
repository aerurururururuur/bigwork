#include "application/overlay_layout.hpp"

#include <algorithm>
#include <string>

namespace application {

void prepareTitleOverlay(OverlayModel& overlay, int total_cols, int total_rows) {
    overlay.active = true;
    overlay.lines = {
        "Pixel arena combat MVP",
        "",
        "[  Start  ]",
        "Enter / click button   Space / hold LMB: fire",
        "Arrows: move   Mouse: aim   Esc: quit   T: theme",
    };

    std::size_t max_len = 0;
    for (const std::string& ln : overlay.lines) {
        max_len = std::max(max_len, ln.size());
    }
    const int box_w = static_cast<int>(max_len) + 4;
    const int box_h = static_cast<int>(overlay.lines.size()) + 2;
    const int ox = std::max(0, (total_cols - box_w) / 2);
    const int oy = std::max(0, (total_rows - box_h) / 2);

    const int start_line = 2;
    const std::string& btn_line = overlay.lines[static_cast<std::size_t>(start_line)];

    overlay.has_start_button = true;
    overlay.start_btn_col = ox + 1;
    overlay.start_btn_row = oy + 1 + start_line;
    overlay.start_btn_width = static_cast<int>(btn_line.size());
    overlay.start_btn_height = 1;
}

void prepareBattleEndOverlay(OverlayModel& overlay, int total_cols, int total_rows, bool victory) {
    overlay.active = true;
    if (victory) {
        overlay.lines = {
            "Victory",
            "",
            "[  OK  ]",
            "Enter: return to title",
        };
    } else {
        overlay.lines = {
            "Defeat",
            "",
            "[  OK  ]",
            "Enter: return to title",
        };
    }

    std::size_t max_len = 0;
    for (const std::string& ln : overlay.lines) {
        max_len = std::max(max_len, ln.size());
    }
    const int box_w = static_cast<int>(max_len) + 4;
    const int box_h = static_cast<int>(overlay.lines.size()) + 2;
    const int ox = std::max(0, (total_cols - box_w) / 2);
    const int oy = std::max(0, (total_rows - box_h) / 2);

    const int start_line = 2;
    const std::string& btn_line = overlay.lines[static_cast<std::size_t>(start_line)];

    overlay.has_start_button = true;
    overlay.start_btn_col = ox + 1;
    overlay.start_btn_row = oy + 1 + start_line;
    overlay.start_btn_width = static_cast<int>(btn_line.size());
    overlay.start_btn_height = 1;
}

bool overlayStartButtonHit(const OverlayModel& overlay, int grid_col, int grid_row) {
    if (!overlay.has_start_button) {
        return false;
    }
    return grid_col >= overlay.start_btn_col && grid_col < overlay.start_btn_col + overlay.start_btn_width &&
           grid_row >= overlay.start_btn_row && grid_row < overlay.start_btn_row + overlay.start_btn_height;
}

} // namespace application
