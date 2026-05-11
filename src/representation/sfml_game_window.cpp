#include "representation/sfml_game_window.hpp"

#include "representation/theme_palette.hpp"

namespace representation {

SfmlGameWindow::SfmlGameWindow(int logical_cols, int logical_rows, int cell_px)
    : cell_px_{cell_px} {
    const unsigned width = static_cast<unsigned>(logical_cols * cell_px);
    const unsigned height = static_cast<unsigned>(logical_rows * cell_px);
    window_.create(sf::VideoMode(width, height), "Yan Tu Gong Lu Xin Bu");
    window_.setVerticalSyncEnabled(true);
    window_.setFramerateLimit(60);

    cell_rect_.setSize(sf::Vector2f(static_cast<float>(cell_px), static_cast<float>(cell_px)));
    cell_rect_.setOutlineThickness(0.f);

    font_ok_ = loadFont();
    if (font_ok_) {
        glyph_.setFont(font_);
        glyph_.setCharacterSize(static_cast<unsigned>(cell_px - 4));
        glyph_.setFillColor(sf::Color::White);
    }
}

bool SfmlGameWindow::loadFont() {
    const std::vector<std::string> paths = {
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/lucon.ttf",
        "assets/fonts/DejaVuSansMono.ttf",
    };
    for (const std::string& path : paths) {
        if (font_.loadFromFile(path)) {
            return true;
        }
    }
    return false;
}

bool SfmlGameWindow::isOpen() const { return window_.isOpen(); }

bool SfmlGameWindow::pollInput(domain::RawInputSnapshot& out) {
    out = domain::RawInputSnapshot{};
    sf::Event event{};
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            return false;
        }
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                out.cancel = true;
            } else if (event.key.code == sf::Keyboard::Enter) {
                out.confirm = true;
            } else if (event.key.code == sf::Keyboard::T) {
                out.toggle_theme = true;
            }
        }
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        out.up = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        out.down = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        out.left = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        out.right = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
        out.cancel = true;
    }
    return true;
}

void SfmlGameWindow::present(int rows, int cols, const std::vector<domain::FrameCell>& cells) {
    window_.clear(sf::Color(12, 14, 22));
    const std::size_t n = static_cast<std::size_t>(rows * cols);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const std::size_t i = static_cast<std::size_t>(r * cols + c);
            if (i >= n || i >= cells.size()) {
                continue;
            }
            const domain::FrameCell& cell = cells[i];
            cell_rect_.setPosition(static_cast<float>(c * cell_px_), static_cast<float>(r * cell_px_));
            cell_rect_.setFillColor(colorFromId(cell.color_id));
            window_.draw(cell_rect_);

            if (font_ok_ && cell.ch != ' ') {
                glyph_.setString(std::string(1, cell.ch));
                const sf::FloatRect bounds = glyph_.getLocalBounds();
                glyph_.setPosition(
                    static_cast<float>(c * cell_px_) + (static_cast<float>(cell_px_) - bounds.width) / 2.f - bounds.left,
                    static_cast<float>(r * cell_px_) + (static_cast<float>(cell_px_) - bounds.height) / 2.f -
                        bounds.top);
                window_.draw(glyph_);
            }
        }
    }
    window_.display();
}

} // namespace representation
