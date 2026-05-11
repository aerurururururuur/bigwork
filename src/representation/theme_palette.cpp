#include "representation/theme_palette.hpp"

namespace representation {

sf::Color colorFromId(std::uint8_t id) {
    switch (id) {
    case 2:
        return sf::Color(180, 70, 70);
    case 3:
        return sf::Color(90, 90, 100);
    case 4:
        return sf::Color(70, 120, 70);
    case 5:
        return sf::Color(240, 210, 80);
    case 6:
        return sf::Color(90, 200, 210);
    case 7:
        return sf::Color(245, 245, 245);
    case 13:
        return sf::Color(80, 110, 180);
    case 14:
        return sf::Color(60, 140, 90);
    default:
        if (id >= 10 && id < 20) {
            return sf::Color(70, 90, 150);
        }
        if (id >= 20) {
            return sf::Color(150, 100, 160);
        }
        return sf::Color(120, 150, 180);
    }
}

} // namespace representation
