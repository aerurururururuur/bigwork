#include "infrastructure/game_config.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <string>

namespace infrastructure {

namespace {

std::string trim(std::string s) {
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    while (!s.empty() && !not_space(static_cast<unsigned char>(s.front()))) {
        s.erase(0, 1);
    }
    while (!s.empty() && !not_space(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    return s;
}

void stripUtf8Bom(std::string& s) {
    if (s.size() >= 3u && static_cast<unsigned char>(s[0]) == 0xEFu && static_cast<unsigned char>(s[1]) == 0xBBu &&
        static_cast<unsigned char>(s[2]) == 0xBFu) {
        s.erase(0, 3);
    }
}

std::filesystem::path ancestorPath(const std::filesystem::path& base, int up_levels) {
    std::filesystem::path p = base;
    for (int i = 0; i < up_levels; ++i) {
        p /= "..";
    }
    return p.lexically_normal();
}

std::optional<std::filesystem::path> resolveAssetFile(const std::filesystem::path& rel_or_abs,
                                                      const std::filesystem::path& ini_path) {
    if (rel_or_abs.empty()) {
        return std::nullopt;
    }
    std::error_code ec;
    if (rel_or_abs.is_absolute()) {
        if (std::filesystem::exists(rel_or_abs, ec)) {
            return rel_or_abs.lexically_normal();
        }
        return std::nullopt;
    }
    const std::filesystem::path ini_dir = ini_path.parent_path();
    std::filesystem::path cand = (ini_dir / rel_or_abs).lexically_normal();
    if (std::filesystem::exists(cand, ec)) {
        return cand;
    }
    const std::filesystem::path cwd = std::filesystem::current_path();
    for (int up = 0; up < 6; ++up) {
        cand = (ancestorPath(cwd, up) / rel_or_abs).lexically_normal();
        if (std::filesystem::exists(cand, ec)) {
            return cand;
        }
    }
    return std::nullopt;
}

} // namespace

static void normalizeWaveRuntime(domain::WaveRuntimeConfig& w) {
    w.mob_waves_before_boss = std::clamp(w.mob_waves_before_boss, 1, 99);
    w.wave_intermission_sec = std::clamp(w.wave_intermission_sec, 0.0, 120.0);
    w.mob_spawn_base_count = std::clamp(w.mob_spawn_base_count, 1, 80);
    w.mob_spawn_per_wave_extra = std::clamp(w.mob_spawn_per_wave_extra, 0, 40);
    w.mob_spawn_max_count = std::max(w.mob_spawn_max_count, w.mob_spawn_base_count);
    w.mob_spawn_max_count = std::clamp(w.mob_spawn_max_count, 1, 128);
    w.scatter_obstacle_base = std::clamp(w.scatter_obstacle_base, 0, 100);
    w.scatter_obstacle_extra_roll = std::clamp(w.scatter_obstacle_extra_roll, 0, 60);
}

static void normalizePlayerDamageTiers(GameConfig& cfg) {
    if (cfg.player_damage_score_tier2 <= cfg.player_damage_score_tier1) {
        cfg.player_damage_score_tier1 = 100;
        cfg.player_damage_mult_tier1 = 1.2f;
        cfg.player_damage_score_tier2 = 300;
        cfg.player_damage_mult_tier2 = 1.5f;
        return;
    }
    cfg.player_damage_mult_tier1 = std::max(1.f, cfg.player_damage_mult_tier1);
    cfg.player_damage_mult_tier2 = std::max(1.f, cfg.player_damage_mult_tier2);
}

static RunMode parseRunMode(std::string val) {
    for (char& c : val) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (val == "development") {
        return RunMode::Development;
    }
    return RunMode::Production;
}

GameConfig loadGameConfigFromIni(const std::filesystem::path& path) {
    GameConfig cfg;
    std::ifstream in(path);
    if (!in) {
        return cfg;
    }
    std::string line;
    while (std::getline(in, line)) {
        std::string t = trim(line);
        stripUtf8Bom(t);
        t = trim(t);
        if (t.empty() || t.front() == '#') {
            continue;
        }
        const auto eq = t.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = trim(t.substr(0, eq));
        const std::string val = trim(t.substr(eq + 1));
        if (key == "background_image" && !val.empty() && !cfg.background_image.has_value()) {
            cfg.background_image = std::filesystem::path(val);
        } else if (key == "music_bgm" && !val.empty() && !cfg.music_bgm.has_value()) {
            cfg.music_bgm = std::filesystem::path(val);
        } else if (key == "music_bgm_boss" && !val.empty() && !cfg.music_bgm_boss.has_value()) {
            cfg.music_bgm_boss = std::filesystem::path(val);
        } else if (key == "music_volume" && !val.empty()) {
            try {
                const int v = std::stoi(val);
                cfg.music_volume = std::clamp(v, 0, 100);
            } catch (...) {
                // keep previous (default or last valid)
            }
        } else if (key == "run_mode" && !val.empty()) {
            cfg.run_mode = parseRunMode(val);
        } else if (key == "player_damage_score_tier1" && !val.empty()) {
            try {
                cfg.player_damage_score_tier1 = std::stoi(val);
            } catch (...) {
            }
        } else if (key == "player_damage_mult_tier1" && !val.empty()) {
            try {
                cfg.player_damage_mult_tier1 = static_cast<float>(std::stod(val));
            } catch (...) {
            }
        } else if (key == "player_damage_score_tier2" && !val.empty()) {
            try {
                cfg.player_damage_score_tier2 = std::stoi(val);
            } catch (...) {
            }
        } else if (key == "player_damage_mult_tier2" && !val.empty()) {
            try {
                cfg.player_damage_mult_tier2 = static_cast<float>(std::stod(val));
            } catch (...) {
            }
        } else if (key == "wave_mob_waves_before_boss" && !val.empty()) {
            try {
                cfg.wave.mob_waves_before_boss = std::stoi(val);
            } catch (...) {
            }
        } else if (key == "wave_intermission_sec" && !val.empty()) {
            try {
                cfg.wave.wave_intermission_sec = std::stod(val);
            } catch (...) {
            }
        } else if (key == "wave_mob_spawn_base" && !val.empty()) {
            try {
                cfg.wave.mob_spawn_base_count = std::stoi(val);
            } catch (...) {
            }
        } else if (key == "wave_mob_spawn_per_wave_extra" && !val.empty()) {
            try {
                cfg.wave.mob_spawn_per_wave_extra = std::stoi(val);
            } catch (...) {
            }
        } else if (key == "wave_mob_spawn_max" && !val.empty()) {
            try {
                cfg.wave.mob_spawn_max_count = std::stoi(val);
            } catch (...) {
            }
        } else if (key == "wave_scatter_base" && !val.empty()) {
            try {
                cfg.wave.scatter_obstacle_base = std::stoi(val);
            } catch (...) {
            }
        } else if (key == "wave_scatter_extra_roll" && !val.empty()) {
            try {
                cfg.wave.scatter_obstacle_extra_roll = std::stoi(val);
            } catch (...) {
            }
        }
    }
    normalizePlayerDamageTiers(cfg);
    normalizeWaveRuntime(cfg.wave);
    return cfg;
}

GameConfig loadGameConfigAuto() {
    std::error_code ec;
    for (int up = 0; up < 6; ++up) {
        const std::filesystem::path ini = ancestorPath(std::filesystem::current_path(), up) / "game_config.ini";
        if (!std::filesystem::exists(ini, ec)) {
            continue;
        }
        GameConfig cfg = loadGameConfigFromIni(ini);
        if (cfg.background_image.has_value()) {
            cfg.background_image = resolveAssetFile(*cfg.background_image, ini);
        }
        if (cfg.music_bgm.has_value()) {
            cfg.music_bgm = resolveAssetFile(*cfg.music_bgm, ini);
        }
        if (cfg.music_bgm_boss.has_value()) {
            cfg.music_bgm_boss = resolveAssetFile(*cfg.music_bgm_boss, ini);
        }
        return cfg;
    }
    return {};
}

} // namespace infrastructure
