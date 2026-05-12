#pragma once

#include <cstdint>

namespace domain {

/** Visual / species id (orthogonal to `EnemyArchetype` AI). */
enum class EnemySpriteId : std::uint8_t { Slime = 0, BugBit = 1, Spookmoth = 2, Pebblin = 3 };

} // namespace domain
