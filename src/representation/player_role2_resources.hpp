#pragma once

#include "representation/sprite_sheet_config.hpp"

#include <SFML/Graphics/Texture.hpp>

#include <string>

namespace representation {

/** One horizontal strip (row 0..rows-1) for Role2 protagonist art. */
struct PlayerRole2Strip {
    sf::Texture texture{};
    SpriteLinearClip clip{};
    int columns{1};
    int rows{1};
    bool load_ok{false};
};

/** Four strips + scale; loaded from `assets/sprites/player_sheet_role2.json`. */
class PlayerRole2Resources {
public:
    static PlayerRole2Resources load_from_file(const std::string& path);

    bool valid() const noexcept { return valid_; }
    float scale_cells() const noexcept { return scale_cells_; }

    const PlayerRole2Strip& idle() const noexcept { return idle_; }
    const PlayerRole2Strip& run() const noexcept { return run_; }
    const PlayerRole2Strip& attack() const noexcept { return attack_; }
    const PlayerRole2Strip& death() const noexcept { return death_; }

    std::string load_error;

private:
    bool valid_{false};
    float scale_cells_{1.5f};
    PlayerRole2Strip idle_{};
    PlayerRole2Strip run_{};
    PlayerRole2Strip attack_{};
    PlayerRole2Strip death_{};
};

} // namespace representation
