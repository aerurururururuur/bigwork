#pragma once

#include <cstdint>

namespace domain {

enum class EnemyArchetype : std::uint8_t { Melee = 0, Ranged = 1, EliteHybrid = 2, Boss = 3, BossMinion = 4 };

} // namespace domain
