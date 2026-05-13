#pragma once

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace representation {

/** Loads per-folder frame sequences from `assets/sprites/bullets/boss/1` through `6`. */
class BossBulletFrameResources {
public:
    void load_from_directory(const std::string& root = "assets/sprites/bullets/boss");

    /** True when at least one strip has frames. */
    bool valid() const noexcept;
    /** `strip` in `0..5` maps to folder `{strip+1}`. */
    bool strip_ready(std::uint8_t strip) const noexcept;
    int frame_count(std::uint8_t strip) const noexcept;
    /** `frame_index` may be any integer; wraps by frame count. */
    const sf::Texture* frame(std::uint8_t strip, int frame_index) const noexcept;

private:
    static constexpr int kNumStrips = 6;
    std::vector<std::vector<sf::Texture>> strips_{};
};

} // namespace representation
