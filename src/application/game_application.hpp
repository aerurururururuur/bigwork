#pragma once

#include "application/game_command.hpp"
#include "application/game_state.hpp"
#include "application/render_snapshot.hpp"

#include "domain/ports/iaudit_sink.hpp"
#include "domain/raw_input.hpp"
#include "domain/world.hpp"
#include "infrastructure/game_config.hpp"

#include <deque>
#include <vector>

namespace application {

class GameApplication {
public:
    GameApplication(domain::World& world, domain::IAuditSink& audit, int logical_cell_px,
                    const infrastructure::GameConfig& cfg);

    GameState state() const { return state_; }

    void tick(double dt, const std::vector<GameCommand>& commands, const domain::RawInputSnapshot& raw);

    RenderSnapshot buildSnapshot();

    bool wantsQuit() const { return quit_requested_; }

private:
    void enqueueDialog(std::vector<std::string> lines);

    domain::World* world_{nullptr};
    domain::IAuditSink* audit_{nullptr};
    GameState state_{GameState::Title};
    ThemeStyle theme_{ThemeStyle::Dusk};
    bool quit_requested_{false};
    int cell_px_{16};
    infrastructure::RunMode run_mode_{infrastructure::RunMode::Production};

    std::deque<std::vector<std::string>> dialog_queue_{};
    bool had_living_boss_{false};
    bool boss_intro_dialog_fired_{false};

    TitleUiPhaseView title_ui_phase_{TitleUiPhaseView::MainMenu};
    int title_main_index_{0};
    int title_char_index_{0};
};

} // namespace application
