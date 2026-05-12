#include "representation/sfml_game_window.hpp"

#include "application/overlay_layout.hpp"
#include "representation/render_constants.hpp"
#include "representation/theme_palette.hpp"

#include <algorithm>
#include <cmath>

namespace representation {

SfmlGameWindow::SfmlGameWindow(int logical_cols, int logical_rows, int cell_px)
    : logical_cols_{logical_cols}, logical_rows_{logical_rows}, cell_px_{cell_px} {
    const unsigned width = static_cast<unsigned>(logical_cols * cell_px);
    const unsigned height = static_cast<unsigned>(logical_rows * cell_px);

    sf::ContextSettings ctx;
    ctx.antialiasingLevel = 0;

    window_.create(sf::VideoMode(width, height), "Pixel arena combat", sf::Style::Default, ctx);
    window_.setVerticalSyncEnabled(true);
    window_.setFramerateLimit(60);

    syncLetterboxView();

    cell_rect_.setSize(sf::Vector2f(static_cast<float>(cell_px), static_cast<float>(cell_px)));
    cell_rect_.setOutlineThickness(0.f);

    micro_rect_.setOutlineThickness(0.f);

    overlay_rect_.setOutlineThickness(0.f);

    bullet_shape_.setFillColor(colorFromId(46));
    bullet_shape_.setOutlineThickness(0.f);

    font_ok_ = loadFont();
    if (font_ok_) {
        overlay_text_.setFont(font_);
        overlay_text_.setFillColor(sf::Color(235, 235, 240));
    }
}

bool SfmlGameWindow::loadFont() {
    const std::vector<std::string> paths = {
        "assets/fonts/DejaVuSansMono.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/lucon.ttf",
    };
    for (const std::string& path : paths) {
        if (font_.loadFromFile(path)) {
            return true;
        }
    }
    return false;
}

bool SfmlGameWindow::isOpen() const { return window_.isOpen(); }

void SfmlGameWindow::syncLetterboxView() {
    const unsigned winW = window_.getSize().x;
    const unsigned winH = window_.getSize().y;
    const float lw = static_cast<float>(logical_cols_ * cell_px_);
    const float lh = static_cast<float>(logical_rows_ * cell_px_);
    if (winW == 0 || winH == 0 || lw <= 0.f || lh <= 0.f) {
        return;
    }
    sf::View view(sf::FloatRect(0.f, 0.f, lw, lh));
    const float wr = static_cast<float>(winW) / lw;
    const float hr = static_cast<float>(winH) / lh;
    const float scale = std::min(wr, hr);
    const float vpW = (lw * scale) / static_cast<float>(winW);
    const float vpH = (lh * scale) / static_cast<float>(winH);
    const float vpL = (1.f - vpW) * 0.5f;
    const float vpT = (1.f - vpH) * 0.5f;
    view.setViewport(sf::FloatRect(vpL, vpT, vpW, vpH));
    window_.setView(view);
}

sf::Vector2f SfmlGameWindow::mapWindowPixelToLogical(sf::RenderWindow& win, sf::Vector2i px) {
    return win.mapPixelToCoords(px);
}

bool SfmlGameWindow::mousePixelInsideClient(const sf::RenderWindow& win, sf::Vector2i px) {
    const auto sz = win.getSize();
    return px.x >= 0 && px.y >= 0 && static_cast<unsigned>(px.x) < sz.x &&
           static_cast<unsigned>(px.y) < sz.y;
}

bool SfmlGameWindow::mouseInputActive() const {
    if (!window_.hasFocus()) {
        return false;
    }
    const sf::Vector2i mp = sf::Mouse::getPosition(window_);
    return mousePixelInsideClient(window_, mp);
}

bool SfmlGameWindow::pollInput(domain::RawInputSnapshot& out, const application::RenderSnapshot& view) {
    syncLetterboxView();

    out = domain::RawInputSnapshot{};
    sf::Event event{};
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            return false;
        }
        if (event.type == sf::Event::Resized) {
            syncLetterboxView();
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
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            const sf::Vector2i epx(event.mouseButton.x, event.mouseButton.y);
            if (window_.hasFocus() && mousePixelInsideClient(window_, epx)) {
                const sf::Vector2f lp = mapWindowPixelToLogical(window_, epx);
                const float lw = static_cast<float>(logical_cols_ * cell_px_);
                const float lh = static_cast<float>(logical_rows_ * cell_px_);
                if (lp.x >= 0.f && lp.y >= 0.f && lp.x < lw && lp.y < lh) {
                    const int col = static_cast<int>(std::floor(lp.x)) / cell_px_;
                    const int row = static_cast<int>(std::floor(lp.y)) / cell_px_;
                    if (application::overlayStartButtonHit(view.overlay, col, row)) {
                        out.pointer_confirm = true;
                    }
                }
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
    if (!view.overlay.active) {
        if (mouseInputActive()) {
            const sf::Vector2i mp = sf::Mouse::getPosition(window_);
            const sf::Vector2f lp = mapWindowPixelToLogical(window_, mp);
            const float lw = static_cast<float>(logical_cols_ * cell_px_);
            const float lh = static_cast<float>(logical_rows_ * cell_px_);
            if (lp.x >= 0.f && lp.y >= 0.f && lp.x < lw && lp.y < lh) {
                out.mouse_px = static_cast<int>(std::lround(lp.x));
                out.mouse_py = static_cast<int>(std::lround(lp.y));
                out.aim_from_mouse = true;
            }
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            out.fire = true;
        }
        if (mouseInputActive() && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            out.fire = true;
        }
    }
    return true;
}

void SfmlGameWindow::drawBullets(const application::RenderSnapshot& snap) {
    const float len = std::max(3.f, static_cast<float>(cell_px_) * 0.55f);
    const float thick = std::max(2.f, static_cast<float>(cell_px_) * 0.22f);
    bullet_shape_.setSize(sf::Vector2f(len, thick));
    bullet_shape_.setOrigin(len * 0.5f, thick * 0.5f);

    for (const auto& b : snap.bullets) {
        const float px = b.world_x * static_cast<float>(cell_px_);
        const float py = b.world_y * static_cast<float>(cell_px_);
        bullet_shape_.setPosition(std::floor(px), std::floor(py));
        bullet_shape_.setRotation(b.rotation_deg);
        window_.draw(bullet_shape_);
    }
}

void SfmlGameWindow::drawOverlay(const application::RenderSnapshot& snap) {
    if (!snap.overlay.active || snap.overlay.lines.empty()) {
        return;
    }

    std::size_t max_len = 0;
    for (const std::string& ln : snap.overlay.lines) {
        max_len = std::max(max_len, ln.size());
    }
    const int box_w = static_cast<int>(max_len) + 4;
    const int box_h = static_cast<int>(snap.overlay.lines.size()) + 2;
    const int ox = std::max(0, (snap.total_cols - box_w) / 2);
    const int oy = std::max(0, (snap.total_rows - box_h) / 2);

    const float px = static_cast<float>(ox * cell_px_);
    const float py = static_cast<float>(oy * cell_px_);
    const float pw = static_cast<float>(box_w * cell_px_);
    const float ph = static_cast<float>(box_h * cell_px_);

    const float border_thick = cell_px_ <= 14 ? 1.f : 2.f;

    overlay_rect_.setSize(sf::Vector2f(std::floor(pw), std::floor(ph)));
    overlay_rect_.setPosition(std::floor(px), std::floor(py));
    overlay_rect_.setFillColor(sf::Color(22, 26, 34, 245));
    overlay_rect_.setOutlineColor(sf::Color(188, 194, 210));
    overlay_rect_.setOutlineThickness(border_thick);
    window_.draw(overlay_rect_);

    if (snap.overlay.has_start_button) {
        const float bx = static_cast<float>(snap.overlay.start_btn_col * cell_px_);
        const float by = static_cast<float>(snap.overlay.start_btn_row * cell_px_);
        const float bw = static_cast<float>(snap.overlay.start_btn_width * cell_px_);
        const float bh = static_cast<float>(snap.overlay.start_btn_height * cell_px_);
        overlay_rect_.setSize(sf::Vector2f(std::floor(bw), std::floor(bh)));
        overlay_rect_.setPosition(std::floor(bx), std::floor(by));
        overlay_rect_.setFillColor(colorFromId(9));
        overlay_rect_.setOutlineThickness(0.f);
        window_.draw(overlay_rect_);
    }

    if (!font_ok_) {
        return;
    }

    const unsigned char_size =
        std::max(7u, static_cast<unsigned>((static_cast<unsigned>(cell_px_) * 42u) / 100u));
    overlay_text_.setCharacterSize(char_size);
    overlay_text_.setFillColor(sf::Color(228, 232, 238));

    const float line_step = static_cast<float>(cell_px_);
    const float text_left = std::floor(px + static_cast<float>(cell_px_));
    float line_y = std::floor(py + static_cast<float>(cell_px_) + static_cast<float>(char_size) * 0.12f);

    for (std::size_t li = 0; li < snap.overlay.lines.size(); ++li) {
        overlay_text_.setString(snap.overlay.lines[li]);
        overlay_text_.setPosition(std::floor(text_left), std::floor(line_y));
        window_.draw(overlay_text_);
        line_y += line_step;
    }
}

void SfmlGameWindow::present(int rows, int cols, const std::vector<domain::FrameCell>& cells,
                               const application::RenderSnapshot& snap, double dt) {
    syncLetterboxView();
    window_.clear(sf::Color(12, 14, 22));
    const std::size_t n = static_cast<std::size_t>(rows * cols);

    const int sky_sub = (kSkyMicroCellsPerAxis > 1 && (cell_px_ % kSkyMicroCellsPerAxis) == 0)
                            ? kSkyMicroCellsPerAxis
                            : 1;
    const int micro_px = cell_px_ / sky_sub;

    cell_rect_.setSize(sf::Vector2f(static_cast<float>(cell_px_), static_cast<float>(cell_px_)));
    micro_rect_.setSize(sf::Vector2f(static_cast<float>(micro_px), static_cast<float>(micro_px)));

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const std::size_t i = static_cast<std::size_t>(r * cols + c);
            if (i >= n || i >= cells.size()) {
                continue;
            }
            const domain::FrameCell& cell = cells[i];
            const int base_x = c * cell_px_;
            const int base_y = r * cell_px_;

            if (r < snap.sky_rows && sky_sub > 1) {
                for (int sy = 0; sy < sky_sub; ++sy) {
                    for (int sx = 0; sx < sky_sub; ++sx) {
                        micro_rect_.setPosition(static_cast<float>(base_x + sx * micro_px),
                                                static_cast<float>(base_y + sy * micro_px));
                        micro_rect_.setFillColor(skyMicroFill(cell.color_id, r, c, sx, sy, sky_sub,
                                                                snap.sky_scroll_slow, snap.sky_scroll_fast));
                        window_.draw(micro_rect_);
                    }
                }
            } else {
                cell_rect_.setPosition(static_cast<float>(base_x), static_cast<float>(base_y));
                cell_rect_.setFillColor(colorFromId(cell.color_id));
                window_.draw(cell_rect_);
            }
        }
    }

    movement_particles_.update(dt, snap, cell_px_);
    movement_particles_.draw(window_);

    drawBullets(snap);
    drawOverlay(snap);
    window_.display();
}

} // namespace representation
