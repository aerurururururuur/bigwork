#pragma once

#include "representation/sprite_sheet_config.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace sf {
class Texture;
}

namespace representation {

struct BossCatStrip {
    sf::Texture texture{};
    SpriteLinearClip clip{};
    int columns{1};
    int rows{1};
    bool load_ok{false};
};

class BossCatResources {
public:
    static BossCatResources load_from_file(const std::string& path);

    bool valid() const noexcept { return valid_; }
    float scale_cells() const noexcept { return scale_cells_; }

    const BossCatStrip& idle() const noexcept { return idle_; }
    const BossCatStrip& run() const noexcept { return run_; }
    const BossCatStrip& attack() const noexcept { return attack_; }
    const BossCatStrip& hurt() const noexcept { return hurt_; }

    std::string load_error;

private:
    bool valid_{false};
    float scale_cells_{2.35f};
    BossCatStrip idle_{};
    BossCatStrip run_{};
    BossCatStrip attack_{};
    BossCatStrip hurt_{};
};

struct BossCatDraw {
    bool ok{false};
    const sf::Texture* texture{nullptr};
    sf::IntRect rect{};
    float scale_x{1.f};
    float scale_y{1.f};
    float origin_x{0.f};
    float origin_y{0.f};
};

bool computeBossCatStripDraw(const BossCatStrip& strip, double anim_clock, int cell_px, float disc_radius_px,
                             float scale_cells, bool face_left, BossCatDraw& out);

} // namespace representation
