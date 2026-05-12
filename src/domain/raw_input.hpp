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
    bool confirm{false};
    bool pointer_confirm{false};
    bool cancel{false};
    bool toggle_theme{false};
};

} // namespace domain
