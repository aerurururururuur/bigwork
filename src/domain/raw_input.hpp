#pragma once

namespace domain {

/** Raw edge from keyboard poll (platform-neutral DTO). */
struct RawInputSnapshot {
    bool up{false};
    bool down{false};
    bool left{false};
    bool right{false};
    bool confirm{false};
    bool cancel{false};
    bool toggle_theme{false};
};

} // namespace domain
