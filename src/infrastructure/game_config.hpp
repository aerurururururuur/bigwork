#pragma once

#include <filesystem>
#include <optional>

namespace infrastructure {

struct GameConfig {
    /** First non-empty `background_image=` from INI, if any. */
    std::optional<std::filesystem::path> background_image;
    /** First non-empty `music_bgm=` from INI, if any (resolved to absolute when possible). */
    std::optional<std::filesystem::path> music_bgm;
    /** Parsed `music_volume=` 0–100; default 70 when key absent or invalid. */
    int music_volume{70};
};

/** Parse simple key=value lines; `#` starts a comment; empty lines ignored. */
GameConfig loadGameConfigFromIni(const std::filesystem::path& path);

/**
 * Search `game_config.ini` from cwd upward (handles exe run from build/Debug while file is at repo root),
 * then resolve relative asset paths against ini directory and the same upward search.
 */
GameConfig loadGameConfigAuto();

} // namespace infrastructure
