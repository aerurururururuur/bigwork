#pragma once

#include "domain/ports/iterminal.hpp"

namespace infrastructure {

/** Win32 WriteConsoleOutput single blit per frame. */
class Win32ConsoleTerminal final : public domain::ITerminal {
public:
    Win32ConsoleTerminal();
    ~Win32ConsoleTerminal() override;

    void writeFrame(int rows, int cols, const std::vector<domain::FrameCell>& cells) override;

private:
    void* console_out_{nullptr}; // HANDLE
    void hideCursor();
    void showCursor();
};

} // namespace infrastructure
