#include "application/overlay_layout.hpp"

#include <algorithm>
#include <string>

namespace application {

namespace {

void setCenteredPanel(OverlayModel& overlay, int total_cols, int total_rows, int start_line,
                      const std::string& btn_line) {
    std::size_t max_len = 0;
    for (const std::string& ln : overlay.lines) {
        max_len = std::max(max_len, ln.size());
    }
    const int box_w = static_cast<int>(max_len) + 4;
    const int box_h = static_cast<int>(overlay.lines.size()) + 2;
    const int ox = std::max(0, (total_cols - box_w) / 2);
    const int oy = std::max(0, (total_rows - box_h) / 2);

    overlay.placement = OverlayPlacement::Centered;
    overlay.panel_col = ox;
    overlay.panel_row = oy;
    overlay.panel_w = box_w;
    overlay.panel_h = box_h;

    overlay.has_start_button = true;
    overlay.start_btn_col = ox + 1;
    overlay.start_btn_row = oy + 1 + start_line;
    overlay.start_btn_width = static_cast<int>(btn_line.size());
    overlay.start_btn_height = 1;
}

} // namespace

void prepareTitleOverlay(OverlayModel& overlay, int total_cols, int total_rows) {
    overlay.active = true;
    overlay.lines = {
        "Pixel arena combat MVP",
        "",
        "[  Start  ]",
        "Enter / click button   Space / hold LMB: fire   Q: ring   E: narrow fan",
        "WASD: move   Mouse: aim   Tab: role   Esc: quit   T: theme",
    };

    const int start_line = 2;
    const std::string& btn_line = overlay.lines[static_cast<std::size_t>(start_line)];
    setCenteredPanel(overlay, total_cols, total_rows, start_line, btn_line);
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

    const int start_line = 2;
    const std::string& btn_line = overlay.lines[static_cast<std::size_t>(start_line)];
    setCenteredPanel(overlay, total_cols, total_rows, start_line, btn_line);
}

void prepareBottomDialogOverlay(OverlayModel& overlay, int total_cols, int total_rows,
                                const std::vector<std::string>& body_lines) {
    overlay.active = true;
    overlay.lines = body_lines;
    overlay.lines.push_back("");
    static const char* kContinue = "[ Continue ]";
    overlay.lines.emplace_back(kContinue);

    std::size_t max_len = 0;
    for (const std::string& ln : overlay.lines) {
        max_len = std::max(max_len, ln.size());
    }
    const std::string& btn_line = overlay.lines.back();

    constexpr int kMarginRows = 1;
    int box_w = static_cast<int>(max_len) + 4;
    box_w = std::min(box_w, std::max(4, total_cols - 2));
    const int btn_w = static_cast<int>(btn_line.size());
    box_w = std::max(box_w, btn_w + 4);

    const int box_h = static_cast<int>(overlay.lines.size()) + 2;
    const int ox = std::max(0, (total_cols - box_w) / 2);
    const int oy = std::max(0, total_rows - box_h - kMarginRows);

    overlay.placement = OverlayPlacement::BottomBar;
    overlay.panel_col = ox;
    overlay.panel_row = oy;
    overlay.panel_w = box_w;
    overlay.panel_h = box_h;

    const int cont_line = static_cast<int>(overlay.lines.size()) - 1;
    overlay.has_start_button = true;
    overlay.start_btn_row = oy + 1 + cont_line;
    overlay.start_btn_width = btn_w;
    overlay.start_btn_height = 1;
    overlay.start_btn_col = ox + 1 + (box_w - 2) - btn_w;
}

bool overlayStartButtonHit(const OverlayModel& overlay, int grid_col, int grid_row) {
    if (!overlay.has_start_button) {
        return false;
    }
    return grid_col >= overlay.start_btn_col && grid_col < overlay.start_btn_col + overlay.start_btn_width &&
           grid_row >= overlay.start_btn_row && grid_row < overlay.start_btn_row + overlay.start_btn_height;
}

} // namespace application
