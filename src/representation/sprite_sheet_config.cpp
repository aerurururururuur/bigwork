#include "representation/sprite_sheet_config.hpp"

#include <fstream>
#include <regex>
#include <sstream>

namespace representation {

namespace {

std::string read_text_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool parse_int_field(const std::string& inner, const std::string& key, int& out) {
    const std::regex re("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    std::smatch m;
    if (!std::regex_search(inner, m, re)) {
        return false;
    }
    out = std::stoi(m[1].str());
    return true;
}

bool append_row_strip(SpriteLinearClip& clip, int row, int start_col, int frame_count, int columns, int rows,
                      std::string& err) {
    if (frame_count <= 0) {
        err = "frame_count must be positive";
        return false;
    }
    if (row < 0 || row >= rows) {
        err = "clip row out of range";
        return false;
    }
    for (int i = 0; i < frame_count; ++i) {
        const int col = start_col + i;
        if (col < 0 || col >= columns) {
            err = "clip column out of range";
            return false;
        }
        clip.frames.push_back(SpriteFrameCell{row, col});
    }
    return true;
}

bool parse_simple_clip(const std::string& json, const char* clip_name, int columns, int rows, SpriteLinearClip& out,
                       std::string& err) {
    const std::regex block_re(std::string("\"") + clip_name + "\"\\s*:\\s*\\{([^}]*)\\}");
    std::smatch m;
    if (!std::regex_search(json, m, block_re)) {
        err = std::string("missing clips.") + clip_name;
        return false;
    }
    const std::string inner = m[1].str();
    int row = 0;
    int start_col = 0;
    int frame_count = 0;
    int fps = 10;
    if (!parse_int_field(inner, "row", row)) {
        err = std::string("clips.") + clip_name + ".row";
        return false;
    }
    if (!parse_int_field(inner, "frame_count", frame_count)) {
        err = std::string("clips.") + clip_name + ".frame_count";
        return false;
    }
    parse_int_field(inner, "start_col", start_col);
    parse_int_field(inner, "fps", fps);
    out = SpriteLinearClip{};
    out.fps = static_cast<float>(fps);
    out.loop = true;
    return append_row_strip(out, row, start_col, frame_count, columns, rows, err);
}

bool parse_death_clip(const std::string& json, int columns, int rows, SpriteLinearClip& out, std::string& err) {
    const std::regex death_block("\"death\"\\s*:\\s*\\{");
    std::smatch dm;
    if (!std::regex_search(json, dm, death_block)) {
        err = "missing clips.death";
        return false;
    }
    const std::size_t start = dm.position() + dm.length();
    int depth = 1;
    std::size_t i = start;
    for (; i < json.size() && depth > 0; ++i) {
        if (json[i] == '{') {
            ++depth;
        } else if (json[i] == '}') {
            --depth;
        }
    }
    if (depth != 0) {
        err = "clips.death brace mismatch";
        return false;
    }
    const std::string inner = json.substr(start, i - start - 1);

    int fps = 12;
    parse_int_field(inner, "fps", fps);

    const std::size_t seg_pos = inner.find("\"segments\"");
    if (seg_pos == std::string::npos) {
        err = "clips.death.segments missing";
        return false;
    }
    const std::string from_seg = inner.substr(seg_pos);
    const std::regex seg_obj_rc(
        "\\{\\s*\"row\"\\s*:\\s*(\\d+)\\s*,\\s*\"start_col\"\\s*:\\s*(\\d+)\\s*,\\s*\"frame_count\"\\s*:\\s*(\\d+)\\s*\\}");
    const std::regex seg_obj_rfc(
        "\\{\\s*\"row\"\\s*:\\s*(\\d+)\\s*,\\s*\"frame_count\"\\s*:\\s*(\\d+)\\s*\\}");
    std::sregex_iterator it(from_seg.begin(), from_seg.end(), seg_obj_rc);
    std::sregex_iterator end;

    out = SpriteLinearClip{};
    out.fps = static_cast<float>(fps);
    out.loop = false;
    out.frames.clear();

    if (it == end) {
        for (std::sregex_iterator it2(from_seg.begin(), from_seg.end(), seg_obj_rfc); it2 != end; ++it2) {
            const int row = std::stoi((*it2)[1].str());
            const int frame_count = std::stoi((*it2)[2].str());
            if (!append_row_strip(out, row, 0, frame_count, columns, rows, err)) {
                return false;
            }
        }
    } else {
        for (; it != end; ++it) {
            const int row = std::stoi((*it)[1].str());
            const int start_col = std::stoi((*it)[2].str());
            const int frame_count = std::stoi((*it)[3].str());
            if (!append_row_strip(out, row, start_col, frame_count, columns, rows, err)) {
                return false;
            }
        }
    }
    if (out.frames.empty()) {
        err = "clips.death has no frames";
        return false;
    }
    return true;
}

} // namespace

SpriteSheetConfig SpriteSheetConfig::load_from_file(const std::string& path) {
    SpriteSheetConfig cfg;
    const std::string json = read_text_file(path);
    if (json.empty()) {
        cfg.load_error = "cannot read file: " + path;
        return cfg;
    }

    try {
        std::smatch m;
        if (!std::regex_search(json, m, std::regex("\"texture\"\\s*:\\s*\"([^\"]+)\""))) {
            cfg.load_error = "missing texture";
            return cfg;
        }
        cfg.texture_path = m[1].str();

        if (!std::regex_search(json, m, std::regex("\"columns\"\\s*:\\s*(\\d+)"))) {
            cfg.load_error = "missing columns";
            return cfg;
        }
        cfg.columns = std::stoi(m[1].str());

        if (!std::regex_search(json, m, std::regex("\"rows\"\\s*:\\s*(\\d+)"))) {
            cfg.load_error = "missing rows";
            return cfg;
        }
        cfg.rows = std::stoi(m[1].str());

        cfg.scale_cells = 1.1f;
        if (std::regex_search(json, m, std::regex("\"scale_cells\"\\s*:\\s*([0-9.+-eE]+)"))) {
            cfg.scale_cells = static_cast<float>(std::stod(m[1].str()));
        }

        if (cfg.columns <= 0 || cfg.rows <= 0) {
            cfg.load_error = "columns/rows must be positive";
            return cfg;
        }

        std::string err;
        if (!parse_simple_clip(json, "idle", cfg.columns, cfg.rows, cfg.idle, err)) {
            cfg.load_error = err;
            return cfg;
        }
        if (!parse_simple_clip(json, "run", cfg.columns, cfg.rows, cfg.run, err)) {
            cfg.load_error = err;
            return cfg;
        }
        if (!parse_death_clip(json, cfg.columns, cfg.rows, cfg.death, err)) {
            cfg.load_error = err;
            return cfg;
        }
        if (cfg.idle.fps <= 0.f || cfg.run.fps <= 0.f || cfg.death.fps <= 0.f) {
            cfg.load_error = "fps must be positive";
            return cfg;
        }
    } catch (const std::exception& ex) {
        cfg.load_error = ex.what();
        return cfg;
    }

    cfg.valid = true;
    cfg.load_error.clear();
    return cfg;
}

} // namespace representation
