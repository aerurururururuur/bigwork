#include "infrastructure/std_random.hpp"

namespace infrastructure {

StdRandom::StdRandom(domain::Seed seed) : engine_(seed.value != 0 ? seed.value : 1) {}

int StdRandom::uniformInt(int minInclusive, int maxInclusive) {
    if (maxInclusive < minInclusive) {
        return minInclusive;
    }
    std::uniform_int_distribution<int> dist(minInclusive, maxInclusive);
    return dist(engine_);
}

std::uint64_t StdRandom::nextU64() { return engine_(); }

} // namespace infrastructure
