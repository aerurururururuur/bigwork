#pragma once

#include "application/render_snapshot.hpp"
#include "domain/ports/iterminal.hpp"
#include "domain/raw_input.hpp"
#include "representation/movement_particles.hpp"

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

namespace representation {

/** Standalone SFML window: polls input and presents a scaled logical pixel grid (solid fills). */
class SfmlGameWindow {
public:
    SfmlGameWindow(int logical_cols, int logical_rows, int cell_px);

    bool isOpen() const;
    /** Returns false when the user closed the window. */
    bool pollInput(domain::RawInputSnapshot& out, const application::RenderSnapshot& view);
    void present(int rows, int cols, const std::vector<domain::FrameCell>& cells,
                   const application::RenderSnapshot& snap, double dt);

private:
    bool loadFont();
    void syncLetterboxView();
    static sf::Vector2f mapWindowPixelToLogical(sf::RenderWindow& win, sf::Vector2i px);
    static bool mousePixelInsideClient(const sf::RenderWindow& win, sf::Vector2i px);
    bool mouseInputActive() const;

    void drawOverlay(const application::RenderSnapshot& snap);
    void drawBullets(const application::RenderSnapshot& snap);

    MovementParticleSystem movement_particles_;

    sf::RenderWindow window_;
    sf::Font font_;
    bool font_ok_{false};
    int logical_cols_{0};
    int logical_rows_{0};
    int cell_px_{16};
    sf::RectangleShape cell_rect_;
    sf::RectangleShape micro_rect_;
    sf::RectangleShape overlay_rect_;
    sf::RectangleShape bullet_shape_;
    sf::Text overlay_text_;
};

} // namespace representation
