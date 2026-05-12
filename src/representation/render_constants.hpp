#pragma once

#include <cstdint>

namespace representation {

/** Screen pixels per domain logical cell edge (square tiles, no stretch). */
inline constexpr int kScreenPixelsPerLogicalCell = 10;

/**
 * Sky-only: each logical sky cell is drawn as kSkyMicroCellsPerAxis x kSkyMicroCellsPerAxis
 * micro-squares for finer gradients. Requires (cell_px % kSkyMicroCellsPerAxis == 0).
 */
inline constexpr int kSkyMicroCellsPerAxis = 2;

/** Max simultaneous foot-dust particles (visual only). */
inline constexpr int kMovementParticleCap = 256;
/** New particles per frame when a grid step occurred (discrete move). */
inline constexpr int kMovementParticlesSpawnOnStep = 4;

/** Max combat burst particles (hit / death). */
inline constexpr int kCombatVfxParticleCap = 320;

/** World-units/sec: below this, enemy draw uses idle frame (matches movement epsilon order of magnitude). */
inline constexpr float kEnemyAnimMoveEpsilonWorld = 0.02f;

/** Snapshot `BulletView::enemy_bullet_sprite` when drawing Pebblin rock (see `domain::EnemyBulletSprite`). */
inline constexpr std::uint8_t kEnemyBulletViewPebblinRock = 1;

} // namespace representation
