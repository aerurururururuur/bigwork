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
    SkillQ,
    Confirm,
    ToggleTheme,
    Quit,
};

} // namespace application
