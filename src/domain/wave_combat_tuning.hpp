#pragma once

/** Central tuning: mob waves, player bullet range, light combat pressure (tweak for balance / demo). */
namespace domain::wave_combat {

inline constexpr int kMobWavesBeforeBoss = 3;
inline constexpr double kWaveIntermissionSec = 2.0;

inline constexpr int kMobSpawnBaseCount = 5;
inline constexpr int kMobSpawnPerWaveExtra = 2;
inline constexpr int kMobSpawnMaxCount = 16;

inline constexpr int kScatterObstacleBase = 9;
inline constexpr int kScatterObstacleExtraRoll = 5;

inline constexpr float kPlayerMobBulletTravelWorld = 4.5f;
/** Player bullets after Boss spawns; mob-ranged enemy bullets use `kPlayerMobBulletTravelWorld` max range too. */
inline constexpr float kPlayerBossBulletTravelWorld = 28.f;

// Enemy HP / fire cadence (global slight bump vs old baseline).
inline constexpr int kMobMeleeHp = 6;
inline constexpr int kMobRangedHp = 5;
inline constexpr int kMobEliteHp = 12;
inline constexpr int kMobBossHp = 220;
inline constexpr double kMobMeleeFirePeriod = 0.95;
inline constexpr double kMobRangedFirePeriod = 0.70;
inline constexpr double kMobEliteFirePeriod = 0.48;
inline constexpr double kMobBossFirePeriod = 0.45;

// enemy_behavior.cpp mob-phase motion / bullets.
inline constexpr float kEnemyChaseSpeed = 3.05f;
inline constexpr float kEnemyWanderSpeed = 1.05f;
inline constexpr float kRangedKiteSpeed = 2.5f;
inline constexpr float kEliteChaseSpeed = 3.5f;
inline constexpr float kEnemyMobBulletSpeed = 9.5f;
inline constexpr int kEnemyMobBulletDamage = 2;

} // namespace domain::wave_combat
