#pragma once

namespace domain {

/** Boss HP ratio gates (maxHp-scaled): above high = early pattern; below low = late (same spell card, thresholds reserved). */
inline constexpr float kBossHpRatioPhaseHigh = 0.7f;
inline constexpr float kBossHpRatioPhaseLow = 0.4f;

inline constexpr double kBossSpellWindupSec = 0.52;
inline constexpr double kBossSpellRing2DelaySec = 0.26;
inline constexpr double kBossSpellPauseShortSec = 0.20;
inline constexpr double kBossSpellVulnerableSec = 2.2;
inline constexpr float kBossSpellVulnerableDamageMult = 1.5f;

/** Early phase (>70% HP): average seconds between fan / diffusion volleys (jitter in behavior). */
inline constexpr double kBossEarlyRingBurstBaseSec = 2.75;
inline constexpr double kBossEarlyRingBurstMinSec = 1.35;
/** Early diffusion telegraph and gap between ring1 and ring2. */
inline constexpr double kBossEarlyDiffusionWindupSec = 0.68;
inline constexpr double kBossEarlyRing1ToRing2DelaySec = 0.30;

} // namespace domain
