#pragma once

#include "application/game_command.hpp"
#include "application/game_state.hpp"
#include "application/render_snapshot.hpp"

#include "domain/ports/iaudit_sink.hpp"
#include "domain/raw_input.hpp"
#include "domain/world.hpp"
#include "infrastructure/game_config.hpp"

#include <vector>

namespace application {

class GameApplication {
public:
    GameApplication(domain::World& world, domain::IAuditSink& audit, int logical_cell_px,
                    infrastructure::RunMode run_mode = infrastructure::RunMode::Production);

    GameState state() const { return state_; }

    void tick(double dt, const std::vector<GameCommand>& commands, const domain::RawInputSnapshot& raw);

    RenderSnapshot buildSnapshot();

    bool wantsQuit() const { return quit_requested_; }

private:
    domain::World* world_{nullptr};
    domain::IAuditSink* audit_{nullptr};
    GameState state_{GameState::Title};
    ThemeStyle theme_{ThemeStyle::Dusk};
    bool quit_requested_{false};
    int cell_px_{16};
    infrastructure::RunMode run_mode_{infrastructure::RunMode::Production};
};

} // namespace application
