#include "application/game_application.hpp"
#include "application/game_state.hpp"
#include "domain/screen_layout.hpp"
#include "domain/seed.hpp"
#include "domain/world.hpp"
#include "infrastructure/file_audit_sink.hpp"
#include "infrastructure/frame_timer.hpp"
#include "infrastructure/game_config.hpp"
#include "infrastructure/std_random.hpp"
#include "representation/frame_composer.hpp"
#include "representation/key_mapping.hpp"
#include "representation/render_constants.hpp"
#include "representation/bg_music_controller.hpp"
#include "representation/sfml_game_window.hpp"

#include <filesystem>
#include <iostream>
#include <string>

int main() {
    std::error_code ec;
    std::filesystem::create_directories("logs", ec);

    infrastructure::FileAuditSink audit("logs/game_audit.log");
    const infrastructure::GameConfig game_cfg = infrastructure::loadGameConfigAuto();
    if (game_cfg.background_image.has_value()) {
        audit.write("BOOT", std::string("game_config background_image=") +
                                game_cfg.background_image->string());
    }
    if (game_cfg.music_bgm.has_value()) {
        audit.write("BOOT", std::string("game_config music_bgm=") + game_cfg.music_bgm->string());
    }

    infrastructure::FrameTimer timer(60.0);
    domain::Seed seed{42};
    infrastructure::StdRandom rng(seed);

    domain::World world(rng);
    application::GameApplication app(world, audit, representation::kScreenPixelsPerLogicalCell);
    representation::FrameComposer composer;
    representation::SfmlGameWindow window(domain::ScreenLayout::kCols, domain::ScreenLayout::kRows,
                                          representation::kScreenPixelsPerLogicalCell,
                                          game_cfg.background_image);

    representation::BgMusicController bgm;
    if (game_cfg.music_bgm.has_value()) {
        if (!bgm.open(*game_cfg.music_bgm)) {
            audit.write("BOOT", std::string("music_bgm open failed path=") + game_cfg.music_bgm->string());
        } else {
            bgm.setVolume(static_cast<float>(game_cfg.music_volume));
        }
    }

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
        bgm.syncBattleOnly(app.state() == application::GameState::Battle);
        snap = app.buildSnapshot();
        const std::vector<domain::FrameCell> frame = composer.compose(snap);
        window.present(snap.total_rows, snap.total_cols, frame, snap, dt);
    }

    bgm.stop();
    audit.write("SHUTDOWN", "normal exit.");
    audit.flush();
    return 0;
}
