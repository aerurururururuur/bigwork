#pragma once

namespace domain::pickup {

/** Player feet to pickup center distance for pickup (world units). */
inline constexpr float kPickupRadiusWorld = 0.45f;
inline constexpr float kPickupBodyRadiusWorld = 0.12f;

inline constexpr int kRedHeartHealHp = 2;
inline constexpr int kBlueHeartHealMp = 8;

/** Roll in [0,99]: red if < kMobDropRedPct; else blue if < kMobDropRedPct + kMobDropBluePct. */
inline constexpr int kMobDropRedPct = 18;
inline constexpr int kMobDropBluePct = 12;

inline constexpr int kBossDropRedPct = 12;
inline constexpr int kBossDropBluePct = 8;

inline constexpr int kPickupSpawnNudgeTries = 6;

} // namespace domain::pickup
