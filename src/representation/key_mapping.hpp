#pragma once

#include "application/game_command.hpp"
#include "domain/raw_input.hpp"

#include <vector>

namespace representation {

std::vector<application::GameCommand> mapRawInput(const domain::RawInputSnapshot& raw);

} // namespace representation
