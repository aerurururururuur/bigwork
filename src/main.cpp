#include "application/game_application.hpp"
#include "domain/screen_layout.hpp"
#include "domain/seed.hpp"
#include "domain/world.hpp"
#include "infrastructure/file_audit_sink.hpp"
#include "infrastructure/frame_timer.hpp"
#include "infrastructure/std_random.hpp"
#include "representation/frame_composer.hpp"
#include "representation/key_mapping.hpp"
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
    (void)rng;

    domain::World world;
    application::GameApplication app(world, audit);
    representation::FrameComposer composer;
    representation::SfmlGameWindow window(domain::ScreenLayout::kCols, domain::ScreenLayout::kRows, 18);

    audit.write("BOOT", "roadside_stroll window MVP starting.");

    while (window.isOpen() && !app.wantsQuit()) {
        const double dt = timer.waitNextFrame();
        domain::RawInputSnapshot raw{};
        if (!window.pollInput(raw)) {
            break;
        }
        const std::vector<application::GameCommand> cmds = representation::mapRawInput(raw);
        app.tick(dt, cmds);
        const application::RenderSnapshot snap = app.buildSnapshot();
        const std::vector<domain::FrameCell> frame = composer.compose(snap);
        window.present(snap.total_rows, snap.total_cols, frame);
    }

    audit.write("SHUTDOWN", "normal exit.");
    audit.flush();
    return 0;
}
