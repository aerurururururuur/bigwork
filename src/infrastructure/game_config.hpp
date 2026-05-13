#pragma once

#include <filesystem>
#include <optional>

namespace infrastructure {

enum class RunMode {
    Production,
    Development,
};

struct GameConfig {
    /** First non-empty `background_image=` from INI, if any. */
    std::optional<std::filesystem::path> background_image;
    /** First non-empty `music_bgm=` from INI, if any (resolved to absolute when possible). */
    std::optional<std::filesystem::path> music_bgm;
    /** Optional `music_bgm_boss=`; when absent, boss phase uses `music_bgm` if available. */
    std::optional<std::filesystem::path> music_bgm_boss;
    /** Parsed `music_volume=` 0-100; default 70 when key absent or invalid. */
    int music_volume{70};
    /** `run_mode=production` (default) or `development` (Boss digit hotkeys). */
    RunMode run_mode{RunMode::Production};
};

/** Parse simple key=value lines; `#` starts a comment; empty lines ignored. */
GameConfig loadGameConfigFromIni(const std::filesystem::path& path);

/**
 * Search `game_config.ini` from cwd upward (handles exe run from build/Debug while file is at repo root),
 * then resolve relative asset paths against ini directory and the same upward search.
 */
GameConfig loadGameConfigAuto();

} // namespace infrastructure
