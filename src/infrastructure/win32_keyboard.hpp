#pragma once

#include "domain/raw_input.hpp"

namespace infrastructure {

/** Non-blocking poll of arrow keys / Enter / Esc / T (Win32). */
class Win32Keyboard {
public:
    domain::RawInputSnapshot poll() const;
};

} // namespace infrastructure
