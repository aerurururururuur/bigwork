#pragma once

#include <cstdint>
#include <vector>

namespace domain {

enum class CombatVfxKind : std::uint8_t { EnemyDied = 0, PlayerHitByBullet = 1 };

struct CombatVfxEvent {
    CombatVfxKind kind{CombatVfxKind::EnemyDied};
    float world_x{0.f};
    float world_y{0.f};
};

} // namespace domain
