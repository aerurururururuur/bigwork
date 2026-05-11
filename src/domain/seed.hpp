#pragma once

#include <cstdint>

namespace domain {

/** Session / world seed for reproducible RNG-driven rules (MVP: stored, wired to IRandom). */
struct Seed {
    std::uint64_t value{0};
};

} // namespace domain
