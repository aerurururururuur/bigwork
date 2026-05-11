#pragma once

namespace domain {

class IClock {
public:
    virtual ~IClock() = default;
    /** Monotonic seconds since arbitrary origin (for deltaTime). */
    virtual double nowSeconds() const = 0;
};

} // namespace domain
