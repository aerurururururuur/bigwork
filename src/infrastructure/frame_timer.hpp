#pragma once

namespace infrastructure {

/** Fixed FPS pacing; returns delta time in seconds between frames. */
class FrameTimer {
public:
    explicit FrameTimer(double target_fps);

    /** Blocks until next frame boundary; returns dt since last call (first ~1/target). */
    double waitNextFrame();

private:
    double target_fps_;
    double last_{0.0};
    bool first_{true};
};

} // namespace infrastructure
