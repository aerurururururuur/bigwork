#pragma once

#include <vector>

namespace application {

enum class GameCommand {
    None,
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Confirm,
    ToggleTheme,
    Quit,
};

} // namespace application
