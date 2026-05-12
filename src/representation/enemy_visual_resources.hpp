#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace representation {

/** Loads `enemy_visuals.json` + enemy / rock textures; drawing may fall back per-entry. */
class EnemyVisualResources {
public:
    bool load_from_file(const std::string& path);

    const std::string& load_error() const noexcept { return load_error_; }

    /** Idle + move textures present for this `EnemySpriteId` index. */
    bool sprite_ready(int sprite_id) const noexcept;

    const sf::Texture* enemy_idle_tex(int sprite_id) const noexcept;
    const sf::Texture* enemy_move_tex(int sprite_id) const noexcept;
    const sf::Texture* enemy_melee_tex(int sprite_id) const noexcept;
    float enemy_scale_vs_disc(int sprite_id) const noexcept;

    const sf::Texture* pebblin_rock_tex() const noexcept;

private:
    struct Entry {
        sf::Texture idle;
        sf::Texture move;
        sf::Texture melee;
        bool idle_ok{false};
        bool move_ok{false};
        bool melee_ok{false};
        float scale_vs_disc{2.2f};
    };

    std::string load_error_;
    std::vector<Entry> entries_;
    sf::Texture rock_tex_;
    bool rock_ok_{false};
};

} // namespace representation
