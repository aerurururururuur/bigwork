#pragma once

#include <cstdint>

namespace representation {

/** Screen pixels per domain logical cell edge (square tiles, no stretch). */
inline constexpr int kScreenPixelsPerLogicalCell = 10;

/**
 * Nudge player sprite / fallback circle downward in **logical cells** (visual only).
 * Domain feet anchor unchanged; only Representation draw offset.
 */
inline constexpr float kPlayerSpriteDrawDownCells = 1.75f;

/**
 * Role2 only: nudge sprite **up** in logical cells (subtract from draw Y).
 * Feet anchor in Domain unchanged; visual-only.
 */
inline constexpr float kRole2SpriteDrawUpCells = 0.50f;

/**
 * Sky-only: each logical sky cell is drawn as kSkyMicroCellsPerAxis x kSkyMicroCellsPerAxis
 * micro-squares for finer gradients. Requires (cell_px % kSkyMicroCellsPerAxis == 0).
 */
inline constexpr int kSkyMicroCellsPerAxis = 2;

/** Snapshot `BulletView::player_bullet_visual` for Role2 book strip. */
inline constexpr std::uint8_t kPlayerBulletViewRole2Book = 1;
/** Role1 bullet uses `assets/sprites/bullets/red.png`. */
inline constexpr std::uint8_t kPlayerBulletViewRole1Red = 2;

/** `Book.png` strip layout (Role2 book bullet); texture width must divide by columns. */
inline constexpr int kBookBulletStripColumns = 10;
inline constexpr int kBookBulletStripRows = 1;
/** Role2 book bullet: draw height = base_thick * this (was 3.2f; larger = bigger sprite). */
inline constexpr float kRole2BookBulletThicknessMul = 4.75f;

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
/** Boss multi-folder bullet frames (`assets/sprites/bullets/boss/1..6`). */
inline constexpr std::uint8_t kEnemyBulletViewBossBullet = 2;

} // namespace representation
