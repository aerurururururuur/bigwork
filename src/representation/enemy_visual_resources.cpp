#include "representation/enemy_visual_resources.hpp"

#include <fstream>
#include <optional>
#include <regex>
#include <sstream>

namespace representation {

namespace {

std::optional<std::string> json_string_field(const std::string& obj, const char* name) {
    const std::string pat = std::string("\"") + name + "\"\\s*:\\s*\"([^\"]+)\"";
    std::smatch m;
    if (std::regex_search(obj, m, std::regex(pat))) {
        return m[1].str();
    }
    return std::nullopt;
}

std::optional<float> json_float_field(const std::string& obj, const char* name) {
    const std::string pat = std::string("\"") + name + "\"\\s*:\\s*([0-9.+-eE]+)";
    std::smatch m;
    if (std::regex_search(obj, m, std::regex(pat))) {
        return static_cast<float>(std::stod(m[1].str()));
    }
    return std::nullopt;
}

std::optional<int> json_int_field(const std::string& obj, const char* name) {
    const std::string pat = std::string("\"") + name + "\"\\s*:\\s*(-?[0-9]+)";
    std::smatch m;
    if (std::regex_search(obj, m, std::regex(pat))) {
        return std::stoi(m[1].str());
    }
    return std::nullopt;
}

bool extract_bracket_array_body(const std::string& json, const char* key, std::string& out_inner) {
    const std::string needle = std::string("\"") + key + "\"";
    const std::size_t p = json.find(needle);
    if (p == std::string::npos) {
        return false;
    }
    const std::size_t lb = json.find('[', p);
    if (lb == std::string::npos) {
        return false;
    }
    int depth = 0;
    for (std::size_t i = lb; i < json.size(); ++i) {
        if (json[i] == '[') {
            ++depth;
        } else if (json[i] == ']') {
            --depth;
            if (depth == 0) {
                if (i <= lb + 1) {
                    out_inner.clear();
                } else {
                    out_inner = json.substr(lb + 1, i - lb - 1);
                }
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string> split_top_level_json_objects(const std::string& body) {
    std::vector<std::string> out;
    std::size_t i = 0;
    const std::size_t n = body.size();
    while (i < n) {
        const std::size_t ob = body.find('{', i);
        if (ob == std::string::npos) {
            break;
        }
        int depth = 0;
        std::size_t j = ob;
        for (; j < n; ++j) {
            if (body[j] == '{') {
                ++depth;
            } else if (body[j] == '}') {
                --depth;
                if (depth == 0) {
                    out.emplace_back(body.substr(ob, j - ob + 1));
                    i = j + 1;
                    break;
                }
            }
        }
        if (j >= n) {
            break;
        }
    }
    return out;
}

} // namespace

bool EnemyVisualResources::load_from_file(const std::string& path) {
    load_error_.clear();
    entries_.clear();
    entries_.resize(4);
    rock_ok_ = false;

    std::ifstream in(path);
    if (!in) {
        load_error_ = "open failed: " + path;
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string json = ss.str();

    if (const auto rock_path = json_string_field(json, "enemy_bullet_pebblin_rock")) {
        rock_ok_ = rock_tex_.loadFromFile(*rock_path);
        if (!rock_ok_) {
            load_error_ += "rock texture load failed; ";
        }
    }

    std::string arr_body;
    if (!extract_bracket_array_body(json, "entries", arr_body)) {
        load_error_ += "no entries array; ";
        return false;
    }

    const std::vector<std::string> objs = split_top_level_json_objects(arr_body);
    for (const std::string& obj : objs) {
        const std::optional<int> id = json_int_field(obj, "id");
        if (!id || *id < 0 || *id >= static_cast<int>(entries_.size())) {
            continue;
        }
        Entry& e = entries_[static_cast<std::size_t>(*id)];
        if (const auto sc = json_float_field(obj, "scale_vs_disc")) {
            e.scale_vs_disc = *sc;
        }
        if (const auto p = json_string_field(obj, "idle")) {
            e.idle_ok = e.idle.loadFromFile(*p);
        }
        if (const auto p = json_string_field(obj, "move")) {
            e.move_ok = e.move.loadFromFile(*p);
        }
        if (const auto p = json_string_field(obj, "melee_attack")) {
            e.melee_ok = e.melee.loadFromFile(*p);
        }
    }

    return true;
}

bool EnemyVisualResources::sprite_ready(int sprite_id) const noexcept {
    if (sprite_id < 0 || sprite_id >= static_cast<int>(entries_.size())) {
        return false;
    }
    const Entry& e = entries_[static_cast<std::size_t>(sprite_id)];
    return e.idle_ok && e.move_ok;
}

const sf::Texture* EnemyVisualResources::enemy_idle_tex(int sprite_id) const noexcept {
    if (sprite_id < 0 || sprite_id >= static_cast<int>(entries_.size())) {
        return nullptr;
    }
    const Entry& e = entries_[static_cast<std::size_t>(sprite_id)];
    return e.idle_ok ? &e.idle : nullptr;
}

const sf::Texture* EnemyVisualResources::enemy_move_tex(int sprite_id) const noexcept {
    if (sprite_id < 0 || sprite_id >= static_cast<int>(entries_.size())) {
        return nullptr;
    }
    const Entry& e = entries_[static_cast<std::size_t>(sprite_id)];
    return e.move_ok ? &e.move : nullptr;
}

const sf::Texture* EnemyVisualResources::enemy_melee_tex(int sprite_id) const noexcept {
    if (sprite_id < 0 || sprite_id >= static_cast<int>(entries_.size())) {
        return nullptr;
    }
    const Entry& e = entries_[static_cast<std::size_t>(sprite_id)];
    return e.melee_ok ? &e.melee : nullptr;
}

float EnemyVisualResources::enemy_scale_vs_disc(int sprite_id) const noexcept {
    if (sprite_id < 0 || sprite_id >= static_cast<int>(entries_.size())) {
        return 2.2f;
    }
    return entries_[static_cast<std::size_t>(sprite_id)].scale_vs_disc;
}

const sf::Texture* EnemyVisualResources::pebblin_rock_tex() const noexcept {
    return rock_ok_ ? &rock_tex_ : nullptr;
}

} // namespace representation
