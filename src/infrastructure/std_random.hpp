#pragma once

#include "domain/ports/irandom.hpp"
#include "domain/seed.hpp"

#include <random>

namespace infrastructure {

class StdRandom final : public domain::IRandom {
public:
    explicit StdRandom(domain::Seed seed);

    int uniformInt(int minInclusive, int maxInclusive) override;
    std::uint64_t nextU64() override;

private:
    std::mt19937_64 engine_;
};

} // namespace infrastructure
