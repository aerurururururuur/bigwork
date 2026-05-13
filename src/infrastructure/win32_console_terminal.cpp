#include "infrastructure/win32_console_terminal.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <vector>
#endif

namespace infrastructure {

#if defined(_WIN32)

namespace {

WORD mapColor(std::uint8_t id) {
    const WORD fg_white = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    const WORD fg_gray = FOREGROUND_BLUE | FOREGROUND_GREEN;
    const WORD fg_yellow = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    const WORD fg_cyan = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    const WORD fg_magenta = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    const WORD fg_green = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    const WORD fg_red = FOREGROUND_RED | FOREGROUND_INTENSITY;
    const WORD fg_blue = FOREGROUND_BLUE | FOREGROUND_INTENSITY;

    switch (id) {
    case 2:
        return fg_red;
    case 3:
        return fg_gray;
    case 4:
        return FOREGROUND_GREEN;
    case 5:
        return fg_yellow;
    case 6:
        return fg_cyan;
    case 7:
        return fg_white | FOREGROUND_INTENSITY;
    case 8:
        return FOREGROUND_INTENSITY;
    case 9:
        return fg_blue | FOREGROUND_INTENSITY;
    case 13:
        return fg_blue | FOREGROUND_INTENSITY;
    case 14:
        return fg_green;
    default:
        if (id >= 10 && id < 20) {
            return fg_blue | FOREGROUND_INTENSITY;
        }
        if (id >= 20) {
            return fg_magenta;
        }
        return fg_cyan;
    }
}

} // namespace

Win32ConsoleTerminal::Win32ConsoleTerminal() : console_out_(GetStdHandle(STD_OUTPUT_HANDLE)) {
    hideCursor();
}

Win32ConsoleTerminal::~Win32ConsoleTerminal() { showCursor(); }

void Win32ConsoleTerminal::hideCursor() {
    HANDLE h = static_cast<HANDLE>(console_out_);
    CONSOLE_CURSOR_INFO ci{};
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(h, &ci);
}

void Win32ConsoleTerminal::showCursor() {
    HANDLE h = static_cast<HANDLE>(console_out_);
    CONSOLE_CURSOR_INFO ci{};
    ci.dwSize = 1;
    ci.bVisible = TRUE;
    SetConsoleCursorInfo(h, &ci);
}

void Win32ConsoleTerminal::writeFrame(int rows, int cols, const std::vector<domain::FrameCell>& cells) {
    HANDLE h = static_cast<HANDLE>(console_out_);
    if (h == INVALID_HANDLE_VALUE || rows <= 0 || cols <= 0) {
        return;
    }
    const COORD buffer_size{static_cast<SHORT>(cols), static_cast<SHORT>(rows)};
    SetConsoleScreenBufferSize(h, buffer_size);

    SMALL_RECT window_rect{0, 0, static_cast<SHORT>(cols - 1), static_cast<SHORT>(rows - 1)};
    SetConsoleWindowInfo(h, TRUE, &window_rect);

    std::vector<CHAR_INFO> chi(static_cast<std::size_t>(rows * cols));
    const std::size_t n = static_cast<std::size_t>(rows * cols);
    for (std::size_t i = 0; i < n && i < cells.size(); ++i) {
        chi[i].Char.AsciiChar = cells[i].ch;
        chi[i].Attributes = mapColor(cells[i].color_id);
    }

    COORD buf_coord{0, 0};
    SMALL_RECT write_region = window_rect;
    WriteConsoleOutputA(h, chi.data(), buffer_size, buf_coord, &write_region);
}

#else

Win32ConsoleTerminal::Win32ConsoleTerminal() = default;
Win32ConsoleTerminal::~Win32ConsoleTerminal() = default;
void Win32ConsoleTerminal::hideCursor() {}
void Win32ConsoleTerminal::showCursor() {}
void Win32ConsoleTerminal::writeFrame(int, int, const std::vector<domain::FrameCell>&) {}

#endif

} // namespace infrastructure
