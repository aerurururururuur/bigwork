#pragma once

#include <vector>

namespace application {

enum class GameCommand {
    None,
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Fire,
    Confirm,
    ToggleTheme,
    Quit,
};

} // namespace application
