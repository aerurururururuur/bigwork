#include "infrastructure/steady_clock.hpp"

#include <chrono>

namespace infrastructure {

double SteadyClock::nowSeconds() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

} // namespace infrastructure
