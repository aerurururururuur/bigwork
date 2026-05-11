#include "infrastructure/frame_timer.hpp"

#include <chrono>
#include <thread>

namespace infrastructure {

FrameTimer::FrameTimer(double target_fps) : target_fps_{target_fps} {}

double FrameTimer::waitNextFrame() {
    using clock = std::chrono::steady_clock;
    const auto now = clock::now();
    if (first_) {
        first_ = false;
        last_ = std::chrono::duration<double>(now.time_since_epoch()).count();
        const double frame = 1.0 / target_fps_;
        std::this_thread::sleep_for(std::chrono::duration<double>(frame));
        const double after = std::chrono::duration<double>(clock::now().time_since_epoch()).count();
        const double dt = after - last_;
        last_ = after;
        return dt;
    }
    const double frame = 1.0 / target_fps_;
    const double now_s = std::chrono::duration<double>(now.time_since_epoch()).count();
    const double elapsed = now_s - last_;
    if (elapsed < frame) {
        std::this_thread::sleep_for(std::chrono::duration<double>(frame - elapsed));
    }
    const double after = std::chrono::duration<double>(clock::now().time_since_epoch()).count();
    const double dt = after - last_;
    last_ = after;
    return dt;
}

} // namespace infrastructure
