#include "application/game_application.hpp"
#include "domain/screen_layout.hpp"
#include "domain/seed.hpp"
#include "domain/world.hpp"
#include "infrastructure/file_audit_sink.hpp"
#include "infrastructure/frame_timer.hpp"
#include "infrastructure/std_random.hpp"
#include "representation/frame_composer.hpp"
#include "representation/key_mapping.hpp"
#include "representation/render_constants.hpp"
#include "representation/sfml_game_window.hpp"

#include <filesystem>
#include <iostream>

int main() {
    std::error_code ec;
    std::filesystem::create_directories("logs", ec);

    infrastructure::FileAuditSink audit("logs/game_audit.log");
    infrastructure::FrameTimer timer(60.0);
    domain::Seed seed{42};
    infrastructure::StdRandom rng(seed);

    domain::World world(rng);
    application::GameApplication app(world, audit, representation::kScreenPixelsPerLogicalCell);
    representation::FrameComposer composer;
    representation::SfmlGameWindow window(domain::ScreenLayout::kCols, domain::ScreenLayout::kRows,
                                          representation::kScreenPixelsPerLogicalCell);

    audit.write("BOOT", "pixel arena combat window starting.");

    application::RenderSnapshot snap = app.buildSnapshot();

    while (window.isOpen() && !app.wantsQuit()) {
        const double dt = timer.waitNextFrame();
        domain::RawInputSnapshot raw{};
        if (!window.pollInput(raw, snap)) {
            break;
        }
        const std::vector<application::GameCommand> cmds = representation::mapRawInput(raw);
        app.tick(dt, cmds, raw);
        snap = app.buildSnapshot();
        const std::vector<domain::FrameCell> frame = composer.compose(snap);
        window.present(snap.total_rows, snap.total_cols, frame, snap, dt);
    }

    audit.write("SHUTDOWN", "normal exit.");
    audit.flush();
    return 0;
}
