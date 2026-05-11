#include "infrastructure/win32_keyboard.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace infrastructure {

domain::RawInputSnapshot Win32Keyboard::poll() const {
    domain::RawInputSnapshot s{};
#if defined(_WIN32)
    auto down = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
    s.up = down(VK_UP);
    s.down = down(VK_DOWN);
    s.left = down(VK_LEFT);
    s.right = down(VK_RIGHT);
    s.confirm = down(VK_RETURN);
    s.cancel = down(VK_ESCAPE);
    s.toggle_theme = down('T') || down('t');
#endif
    return s;
}

} // namespace infrastructure
