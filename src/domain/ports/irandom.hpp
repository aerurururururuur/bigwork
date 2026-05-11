#pragma once

#include <cstdint>

namespace domain {

class IRandom {
public:
    virtual ~IRandom() = default;
    virtual int uniformInt(int minInclusive, int maxInclusive) = 0;
    virtual std::uint64_t nextU64() = 0;
};

} // namespace domain
