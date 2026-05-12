#pragma once

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

} // namespace representation
