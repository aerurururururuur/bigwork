#pragma once

#include "application/game_command.hpp"
#include "application/game_state.hpp"
#include "application/render_snapshot.hpp"

#include "domain/ports/iaudit_sink.hpp"
#include "domain/world.hpp"

#include <vector>

namespace application {

class GameApplication {
public:
    GameApplication(domain::World& world, domain::IAuditSink& audit);

    GameState state() const { return state_; }

    void tick(double /*dt*/, const std::vector<GameCommand>& commands);

    RenderSnapshot buildSnapshot() const;

    bool wantsQuit() const { return quit_requested_; }

private:
    domain::World* world_{nullptr};
    domain::IAuditSink* audit_{nullptr};
    GameState state_{GameState::Title};
    ThemeStyle theme_{ThemeStyle::Dusk};
    bool quit_requested_{false};
};

} // namespace application
