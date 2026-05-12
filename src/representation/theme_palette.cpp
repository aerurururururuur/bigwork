#include "representation/theme_palette.hpp"

namespace representation {

namespace {

sf::Color duskSkyBand(std::uint8_t id) {
    // ids 50-65: horizon glow to upper dusk
    static const sf::Color kTable[16] = {
        sf::Color(255, 190, 120),
        sf::Color(248, 175, 110),
        sf::Color(235, 155, 95),
        sf::Color(220, 130, 88),
        sf::Color(200, 110, 95),
        sf::Color(175, 95, 115),
        sf::Color(150, 85, 130),
        sf::Color(130, 78, 140),
        sf::Color(115, 72, 145),
        sf::Color(100, 68, 150),
        sf::Color(90, 65, 152),
        sf::Color(82, 62, 148),
        sf::Color(75, 60, 142),
        sf::Color(70, 58, 135),
        sf::Color(65, 56, 128),
        sf::Color(60, 54, 118),
    };
    const int i = static_cast<int>(id - 50);
    if (i >= 0 && i < 16) {
        return kTable[i];
    }
    return kTable[8];
}

sf::Color nightSkyBand(std::uint8_t id) {
    // ids 30-45: deep blue / violet parallax bands
    static const sf::Color kTable[16] = {
        sf::Color(25, 35, 72),
        sf::Color(28, 40, 82),
        sf::Color(32, 46, 92),
        sf::Color(36, 52, 102),
        sf::Color(40, 56, 110),
        sf::Color(38, 50, 105),
        sf::Color(45, 55, 118),
        sf::Color(50, 58, 125),
        sf::Color(42, 48, 108),
        sf::Color(48, 54, 120),
        sf::Color(55, 62, 132),
        sf::Color(35, 42, 95),
        sf::Color(30, 38, 88),
        sf::Color(52, 60, 128),
        sf::Color(44, 52, 115),
        sf::Color(58, 65, 135),
    };
    const int i = static_cast<int>(id - 30);
    if (i >= 0 && i < 16) {
        return kTable[i];
    }
    return kTable[4];
}

} // namespace

sf::Color colorFromId(std::uint8_t id) {
    if (id >= 50 && id <= 65) {
        return duskSkyBand(id);
    }
    if (id >= 30 && id <= 45) {
        return nightSkyBand(id);
    }
    switch (id) {
    case 40:
        return sf::Color(35, 42, 55);
    case 41:
        return sf::Color(120, 118, 130);
    case 42:
        return sf::Color(160, 95, 55);
    case 43:
        return sf::Color(200, 70, 90);
    case 44:
        return sf::Color(210, 130, 60);
    case 45:
        return sf::Color(150, 80, 200);
    case 46:
        return sf::Color(255, 240, 120);
    case 47:
        return sf::Color(80, 200, 255);
    case 48:
        return sf::Color(28, 36, 48);
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
    case 8:
        return sf::Color(28, 32, 42);
    case 9:
        return sf::Color(70, 130, 220);
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

sf::Color skyMicroFill(std::uint8_t sky_color_id, int row, int col, int sub_x, int sub_y, int sub_n,
                       int scroll_slow, int scroll_fast) {
    (void)sub_n;
    const sf::Color base = colorFromId(sky_color_id);
    const int h = (row * 47 + col * 19 + sub_x * 7 + sub_y * 13 + scroll_slow * 3) ^
                    (scroll_fast * 5 + sub_x * sub_y * 11);
    const int bias = (h & 7) - 3;
    auto clamp255 = [](int v) {
        if (v < 0) {
            return 0;
        }
        if (v > 255) {
            return 255;
        }
        return v;
    };
    const int rb = bias + ((h >> 2) & 1);
    const int gb = bias - ((h >> 1) & 1);
    const int bb = bias + ((h >> 3) & 1);
    return sf::Color(static_cast<sf::Uint8>(clamp255(static_cast<int>(base.r) + rb)),
                     static_cast<sf::Uint8>(clamp255(static_cast<int>(base.g) + gb)),
                     static_cast<sf::Uint8>(clamp255(static_cast<int>(base.b) + bb)));
}

} // namespace representation
