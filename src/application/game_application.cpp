#include "application/game_application.hpp"

#include "domain/screen_layout.hpp"

namespace application {

GameApplication::GameApplication(domain::World& world, domain::IAuditSink& audit)
    : world_{&world}, audit_{&audit} {
    audit_->write("SESSION", "GameApplication constructed (MVP title -> roaming).");
}

void GameApplication::tick(double /*dt*/, const std::vector<GameCommand>& commands) {
    for (GameCommand cmd : commands) {
        switch (cmd) {
        case GameCommand::None:
            break;
        case GameCommand::Quit:
            audit_->write("INPUT", "Quit requested.");
            quit_requested_ = true;
            break;
        case GameCommand::ToggleTheme:
            theme_ = (theme_ == ThemeStyle::Dusk) ? ThemeStyle::ColdNight : ThemeStyle::Dusk;
            audit_->write("THEME", theme_ == ThemeStyle::Dusk ? "dusk" : "cold_night");
            break;
        case GameCommand::Confirm:
            if (state_ == GameState::Title) {
                state_ = GameState::Roaming;
                audit_->write("STATE", "Enter roaming.");
            }
            break;
        case GameCommand::MoveUp:
        case GameCommand::MoveDown:
        case GameCommand::MoveLeft:
        case GameCommand::MoveRight:
            if (state_ != GameState::Roaming) {
                break;
            }
            if (cmd == GameCommand::MoveUp) {
                world_->tryMovePlayer(0, -1);
            } else if (cmd == GameCommand::MoveDown) {
                world_->tryMovePlayer(0, 1);
            } else if (cmd == GameCommand::MoveLeft) {
                world_->tryMovePlayer(-1, 0);
            } else if (cmd == GameCommand::MoveRight) {
                world_->tryMovePlayer(1, 0);
            }
            break;
        }
    }
}

RenderSnapshot GameApplication::buildSnapshot() const {
    using domain::ScreenLayout;

    RenderSnapshot s;
    s.total_rows = ScreenLayout::kRows;
    s.total_cols = ScreenLayout::kCols;
    s.sky_rows = ScreenLayout::skyRows();
    s.playfield_rows = ScreenLayout::playfieldRows();
    const int scroll_base = static_cast<int>(world_->scrollWorld());
    s.sky_scroll_slow = scroll_base / 3;
    s.sky_scroll_fast = scroll_base / 1;

    const auto& pf = world_->playfield();
    s.playfield_tiles.resize(static_cast<std::size_t>(pf.width() * pf.height()));
    for (int y = 0; y < pf.height(); ++y) {
        for (int x = 0; x < pf.width(); ++x) {
            s.playfield_tiles[static_cast<std::size_t>(y * pf.width() + x)] = pf.tile(x, y);
        }
    }
    s.player_x = world_->playerGx();
    s.player_y = world_->playerGy();
    s.npc_x = world_->npc().gx;
    s.npc_y = world_->npc().gy;
    s.theme = theme_;

    if (state_ == GameState::Title) {
        s.overlay.active = true;
        s.overlay.lines = {
            "  Yan Tu Gong Lu Xin Bu  ",
            "  Enter: begin   Esc: quit  ",
            "  Arrows move (roaming)  T: theme  ",
        };
    } else {
        s.overlay.active = false;
    }
    return s;
}

} // namespace application
