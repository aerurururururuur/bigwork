#pragma once

#include "representation/sprite_sheet_config.hpp"

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace representation {

/** Loads `enemy_visuals.json` + per-species sheet JSON / atlas texture; rock bullet stays single image. */
class EnemyVisualResources {
public:
    bool load_from_file(const std::string& path);

    const std::string& load_error() const noexcept { return load_error_; }

    bool sprite_ready(int sprite_id) const noexcept;

    const EnemySheetConfig* sheet_config(int sprite_id) const noexcept;
    const sf::Texture* sheet_texture(int sprite_id) const noexcept;

    const sf::Texture* pebblin_rock_tex() const noexcept;

private:
    struct Entry {
        EnemySheetConfig cfg{};
        sf::Texture texture{};
        bool sheet_ok{false};
    };

    std::string load_error_;
    std::vector<Entry> entries_;
    sf::Texture rock_tex_;
    bool rock_ok_{false};
};

} // namespace representation
