#pragma once

#include "domain/ports/iclock.hpp"

namespace infrastructure {

class SteadyClock final : public domain::IClock {
public:
    double nowSeconds() const override;
};

} // namespace infrastructure
