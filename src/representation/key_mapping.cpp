#include "representation/key_mapping.hpp"

namespace representation {

std::vector<application::GameCommand> mapRawInput(const domain::RawInputSnapshot& raw) {
    std::vector<application::GameCommand> out;
    if (raw.cancel) {
        out.push_back(application::GameCommand::Quit);
        return out;
    }
    if (raw.confirm || raw.pointer_confirm) {
        out.push_back(application::GameCommand::Confirm);
    }
    if (raw.toggle_theme) {
        out.push_back(application::GameCommand::ToggleTheme);
    }
    if (raw.up) {
        out.push_back(application::GameCommand::MoveUp);
    }
    if (raw.down) {
        out.push_back(application::GameCommand::MoveDown);
    }
    if (raw.left) {
        out.push_back(application::GameCommand::MoveLeft);
    }
    if (raw.right) {
        out.push_back(application::GameCommand::MoveRight);
    }
    if (raw.fire) {
        out.push_back(application::GameCommand::Fire);
    }
    if (raw.skill_q) {
        out.push_back(application::GameCommand::SkillQ);
    }
    if (out.empty()) {
        out.push_back(application::GameCommand::None);
    }
    return out;
}

} // namespace representation
