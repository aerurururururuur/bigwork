#include "infrastructure/game_config.hpp"

#include <fstream>
#include <gtest/gtest.h>

namespace {

std::filesystem::path writeTempIni(const std::string& content) {
    const auto base = std::filesystem::temp_directory_path();
    const auto path = base / "roadside_stroll_game_config_test.ini";
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
    return path;
}

void removeIfExists(const std::filesystem::path& p) {
    std::error_code ec;
    std::filesystem::remove(p, ec);
}

} // namespace

TEST(GameConfigIni, ParsesWaveKeysAndNormalizes) {
    const auto path = writeTempIni(
        "wave_mob_waves_before_boss=10\n"
        "wave_intermission_sec=3.5\n"
        "wave_mob_spawn_base=4\n"
        "wave_mob_spawn_per_wave_extra=1\n"
        "wave_mob_spawn_max=12\n"
        "wave_scatter_base=2\n"
        "wave_scatter_extra_roll=1\n");
    const infrastructure::GameConfig cfg = infrastructure::loadGameConfigFromIni(path);
    removeIfExists(path);

    EXPECT_EQ(cfg.wave.mob_waves_before_boss, 10);
    EXPECT_DOUBLE_EQ(cfg.wave.wave_intermission_sec, 3.5);
    EXPECT_EQ(cfg.wave.mob_spawn_base_count, 4);
    EXPECT_EQ(cfg.wave.mob_spawn_per_wave_extra, 1);
    EXPECT_EQ(cfg.wave.mob_spawn_max_count, 12);
    EXPECT_EQ(cfg.wave.scatter_obstacle_base, 2);
    EXPECT_EQ(cfg.wave.scatter_obstacle_extra_roll, 1);
}

TEST(GameConfigIni, ClampsWaveMobWavesToAtLeastOne) {
    const auto path = writeTempIni("wave_mob_waves_before_boss=-5\n");
    const infrastructure::GameConfig cfg = infrastructure::loadGameConfigFromIni(path);
    removeIfExists(path);
    EXPECT_EQ(cfg.wave.mob_waves_before_boss, 1);
}

TEST(GameConfigIni, MobSpawnMaxNotBelowBase) {
    const auto path = writeTempIni(
        "wave_mob_spawn_base=20\n"
        "wave_mob_spawn_max=3\n");
    const infrastructure::GameConfig cfg = infrastructure::loadGameConfigFromIni(path);
    removeIfExists(path);
    EXPECT_EQ(cfg.wave.mob_spawn_base_count, 20);
    EXPECT_GE(cfg.wave.mob_spawn_max_count, cfg.wave.mob_spawn_base_count);
}

TEST(GameConfigIni, MusicVolumeClamped) {
    const auto path = writeTempIni("music_volume=500\n");
    const infrastructure::GameConfig cfg = infrastructure::loadGameConfigFromIni(path);
    removeIfExists(path);
    EXPECT_EQ(cfg.music_volume, 100);
}

TEST(GameConfigIni, RunModeDevelopment) {
    const auto path = writeTempIni("run_mode=development\n");
    const infrastructure::GameConfig cfg = infrastructure::loadGameConfigFromIni(path);
    removeIfExists(path);
    EXPECT_EQ(cfg.run_mode, infrastructure::RunMode::Development);
}

TEST(GameConfigIni, InvalidDamageTiersResetToDefaults) {
    const auto path = writeTempIni(
        "player_damage_score_tier1=400\n"
        "player_damage_score_tier2=200\n");
    const infrastructure::GameConfig cfg = infrastructure::loadGameConfigFromIni(path);
    removeIfExists(path);
    EXPECT_EQ(cfg.player_damage_score_tier1, 100);
    EXPECT_EQ(cfg.player_damage_score_tier2, 300);
}
