#pragma once

#include <string>
#include <vector>

namespace representation {

/** One cell in the sprite sheet grid (0-based row/column indices). */
struct SpriteFrameCell {
    int row{0};
    int col{0};
};

/** A linear sequence of frames (row-major within each segment), with timing. */
struct SpriteLinearClip {
    std::vector<SpriteFrameCell> frames;
    float fps{10.f};
    /** When false, animation stops on the last frame (e.g. death). */
    bool loop{true};
};

/** Loaded from `player_sheet.json` (or compatible); drives player sprite only. */
struct SpriteSheetConfig {
    std::string texture_path;
    int columns{1};
    int rows{1};
    /** Target height in logical cells (width scales uniformly). */
    float scale_cells{1.1f};
    SpriteLinearClip idle;
    SpriteLinearClip run;
    SpriteLinearClip death;
    bool valid{false};
    std::string load_error;

    /** Parse JSON file; on failure `valid` is false and `load_error` is set. */
    static SpriteSheetConfig load_from_file(const std::string& path);
};

} // namespace representation
