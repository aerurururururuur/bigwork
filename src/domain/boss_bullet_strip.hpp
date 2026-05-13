#pragma once

#include <cstdint>

namespace domain {

/** `0..5` maps to `assets/sprites/bullets/boss/{1..6}/` frame folders. */
inline constexpr std::uint8_t kBossBulletStripRing1 = 0;
inline constexpr std::uint8_t kBossBulletStripRing2 = 1;
inline constexpr std::uint8_t kBossBulletStripFan = 2;
inline constexpr std::uint8_t kBossBulletStripSpiral = 3;
inline constexpr std::uint8_t kBossBulletStripDualFan = 4;
/** Soft scatter, cross dual ring, wall volley (single look). */
inline constexpr std::uint8_t kBossBulletStripHeavy = 5;

} // namespace domain
