#pragma once

#include <cstdint>
#include <vector>

namespace domain {

/** Opaque palette id; Infrastructure maps to Win32 attributes or ANSI. */
struct FrameCell {
    char ch{' '};
    std::uint8_t color_id{0};
};

class ITerminal {
public:
    virtual ~ITerminal() = default;
    /** Single-frame blit: full rows*cols cells, row-major. */
    virtual void writeFrame(int rows, int cols, const std::vector<FrameCell>& cells) = 0;
};

} // namespace domain
