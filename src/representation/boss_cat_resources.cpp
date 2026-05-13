#include "representation/boss_cat_resources.hpp"

#include <algorithm>
#include <cmath>
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

bool parse_bool_field(const std::string& inner, const std::string& key, bool& out) {
    const std::regex re("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch m;
    if (!std::regex_search(inner, m, re)) {
        return false;
    }
    out = (m[1].str() == "true");
    return true;
}

bool parse_texture_field(const std::string& inner, std::string& out) {
    const std::regex re("\"texture\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch m;
    if (!std::regex_search(inner, m, re)) {
        return false;
    }
    out = m[1].str();
    return true;
}

bool load_one_strip(const std::string& json, const char* clip_name, BossCatStrip& out, std::string& err) {
    const std::regex block_re(std::string("\"") + clip_name + "\"\\s*:\\s*\\{([^}]*)\\}");
    std::smatch m;
    if (!std::regex_search(json, m, block_re)) {
        err = std::string("missing clips.") + clip_name;
        return false;
    }
    const std::string inner = m[1].str();
    std::string tex_path;
    if (!parse_texture_field(inner, tex_path)) {
        err = std::string("clips.") + clip_name + ".texture";
        return false;
    }
    int columns = 1;
    int rows = 1;
    int fps = 10;
    if (!parse_int_field(inner, "columns", columns)) {
        err = std::string("clips.") + clip_name + ".columns";
        return false;
    }
    parse_int_field(inner, "rows", rows);
    parse_int_field(inner, "fps", fps);
    bool loop = true;
    parse_bool_field(inner, "loop", loop);

    if (!out.texture.loadFromFile(tex_path)) {
        err = "texture load failed: " + tex_path;
        return false;
    }
    const auto sz = out.texture.getSize();
    if (columns <= 0 || rows <= 0 || static_cast<int>(sz.x) % columns != 0 ||
        static_cast<int>(sz.y) % rows != 0) {
        err = "texture grid mismatch: " + tex_path;
        return false;
    }
    const int frame_count = columns * rows;
    out.columns = columns;
    out.rows = rows;
    out.clip = SpriteLinearClip{};
    out.clip.fps = static_cast<float>(fps);
    out.clip.loop = loop;
    out.clip.frames.clear();
    out.clip.frames.reserve(static_cast<std::size_t>(frame_count));
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            out.clip.frames.push_back(SpriteFrameCell{row, col});
        }
    }
    out.load_ok = true;
    return true;
}

int clip_frame_index(const SpriteLinearClip& c, double t) {
    if (c.frames.empty()) {
        return 0;
    }
    const float fps = std::max(0.01f, c.fps);
    const double spf = 1.0 / static_cast<double>(fps);
    int idx = static_cast<int>(t / spf);
    const int n = static_cast<int>(c.frames.size());
    if (n <= 0) {
        return 0;
    }
    if (c.loop) {
        idx = (idx % n + n) % n;
    } else {
        idx = std::clamp(idx, 0, n - 1);
    }
    return idx;
}

} // namespace

BossCatResources BossCatResources::load_from_file(const std::string& path) {
    BossCatResources r{};
    const std::string json = read_text_file(path);
    if (json.empty()) {
        r.load_error = "cannot read " + path;
        return r;
    }
    {
        const std::regex re("\"scale_cells\"\\s*:\\s*([0-9.]+)");
        std::smatch m;
        if (std::regex_search(json, m, re)) {
            r.scale_cells_ = static_cast<float>(std::stod(m[1].str()));
        }
    }

    std::string err;
    if (!load_one_strip(json, "idle", r.idle_, err)) {
        r.load_error = err;
        return r;
    }
    if (!load_one_strip(json, "run", r.run_, err)) {
        r.load_error = err;
        return r;
    }
    if (!load_one_strip(json, "attack", r.attack_, err)) {
        r.load_error = err;
        return r;
    }
    if (!load_one_strip(json, "hurt", r.hurt_, err)) {
        r.load_error = err;
        return r;
    }
    r.valid_ = r.idle_.load_ok && r.run_.load_ok && r.attack_.load_ok && r.hurt_.load_ok;
    if (!r.valid_) {
        r.load_error = "strip incomplete";
    }
    return r;
}

bool computeBossCatStripDraw(const BossCatStrip& strip, double anim_clock, int cell_px, float disc_radius_px,
                             float scale_cells, bool face_left, BossCatDraw& out) {
    out = BossCatDraw{};
    if (!strip.load_ok || strip.clip.frames.empty() || strip.columns <= 0 || strip.rows <= 0) {
        return false;
    }
    const int idx = clip_frame_index(strip.clip, anim_clock);
    const sf::Texture& tex = strip.texture;
    const auto sz = tex.getSize();
    const int fw = static_cast<int>(sz.x) / strip.columns;
    const int fh = static_cast<int>(sz.y) / strip.rows;
    if (fw <= 0 || fh <= 0) {
        return false;
    }
    const int clamped = std::clamp(idx, 0, static_cast<int>(strip.clip.frames.size()) - 1);
    const SpriteFrameCell& fc = strip.clip.frames[static_cast<std::size_t>(clamped)];
    out.rect = sf::IntRect(fc.col * fw, fc.row * fh, fw, fh);
    out.texture = &tex;

    const float frame_h = static_cast<float>(out.rect.height);
    const float frame_w = static_cast<float>(out.rect.width);
    const float cell_f = static_cast<float>(std::max(1, cell_px));
    const float from_json = scale_cells * cell_f;
    const float from_disc = 2.f * disc_radius_px * 1.12f;
    const float target_h = std::max(from_json, from_disc);
    const float scale = target_h / std::max(1.f, frame_h);
    out.scale_x = face_left ? -scale : scale;
    out.scale_y = scale;
    out.origin_x = std::floor(frame_w * 0.5f);
    out.origin_y = frame_h;
    out.ok = true;
    return true;
}

} // namespace representation
