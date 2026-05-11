#pragma once

#include "domain/ports/iterminal.hpp"
#include "domain/raw_input.hpp"

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>

namespace representation {

/** Standalone SFML window: polls input and presents a scaled CharPixel grid. */
class SfmlGameWindow {
public:
    SfmlGameWindow(int logical_cols, int logical_rows, int cell_px);

    bool isOpen() const;
    /** Returns false when the user closed the window. */
    bool pollInput(domain::RawInputSnapshot& out);
    void present(int rows, int cols, const std::vector<domain::FrameCell>& cells);

private:
    bool loadFont();

    sf::RenderWindow window_;
    sf::Font font_;
    bool font_ok_{false};
    int cell_px_{16};
    sf::RectangleShape cell_rect_;
    sf::Text glyph_;
};

} // namespace representation
