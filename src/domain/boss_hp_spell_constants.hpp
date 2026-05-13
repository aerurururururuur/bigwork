#pragma once

namespace domain {

/** Boss HP ratio gates (maxHp-scaled): above high = early pattern; below low = late (same spell card, thresholds reserved). */
inline constexpr float kBossHpRatioPhaseHigh = 0.7f;
inline constexpr float kBossHpRatioPhaseLow = 0.4f;

inline constexpr double kBossSpellWindupSec = 1.0;
inline constexpr double kBossSpellRing2DelaySec = 0.5;
inline constexpr double kBossSpellPauseShortSec = 0.45;
inline constexpr double kBossSpellVulnerableSec = 3.0;
inline constexpr float kBossSpellVulnerableDamageMult = 1.5f;

} // namespace domain
