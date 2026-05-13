#pragma once

#include <cstdint>

namespace domain {

enum class PickupKind : std::uint8_t { RedHeart = 0, BlueHeart = 1 };

struct PickupDrop {
    PickupKind kind{PickupKind::RedHeart};
    float x{0.f};
    float y{0.f};
};

} // namespace domain
