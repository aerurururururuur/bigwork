#pragma once

namespace domain {

/** Raw edge from keyboard poll (platform-neutral DTO). */
struct RawInputSnapshot {
    bool up{false};
    bool down{false};
    bool left{false};
    bool right{false};
    /** When true, `mouse_px`/`mouse_py` are in window client space for aim (battle, no modal overlay). */
    bool aim_from_mouse{false};
    int mouse_px{0};
    int mouse_py{0};
    bool fire{false};
    /** Edge: Q skill (one frame). */
    bool skill_q{false};
    /** Edge: E narrow-fan skill (one frame). */
    bool skill_e{false};
    bool confirm{false};
    bool pointer_confirm{false};
    bool cancel{false};
    bool toggle_theme{false};
    /** Edge: development-only boss skill slot 1-9 (first key this frame). */
    int dev_boss_skill_slot{0};
    /** Edge: development-only: kill all living enemies this frame (key X). */
    bool dev_kill_all_enemies{false};
    /** Edge: Tab - cycle player character skin (battle, no overlay). */
    bool toggle_player_character{false};
    /** Edge: Arrow Up on title menu (one frame). */
    bool title_menu_nav_up{false};
    /** Edge: Arrow Down on title menu (one frame). */
    bool title_menu_nav_down{false};
};

} // namespace domain
