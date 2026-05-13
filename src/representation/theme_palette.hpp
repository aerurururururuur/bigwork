#pragma once

#include <SFML/Graphics/Color.hpp>

#include <cstdint>

namespace representation {

sf::Color colorFromId(std::uint8_t id);

/** Sub-pixel tint inside one logical sky cell (parallax detail; playfield does not use). */
sf::Color skyMicroFill(std::uint8_t sky_color_id, int row, int col, int sub_x, int sub_y, int sub_n,
                       int scroll_slow, int scroll_fast);

} // namespace representation
