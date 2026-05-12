#include "application/game_application.hpp"

#include "application/overlay_layout.hpp"
#include "domain/screen_layout.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kAimEpsilonPx = 2.f;

} // namespace

namespace application {

GameApplication::GameApplication(domain::World& world, domain::IAuditSink& audit, int logical_cell_px)
    : world_{&world}, audit_{&audit}, cell_px_{logical_cell_px > 0 ? logical_cell_px : 16} {
    audit_->write("SESSION", "GameApplication constructed (pixel combat MVP).");
}

void GameApplication::tick(double dt, const std::vector<GameCommand>& commands,
                           const domain::RawInputSnapshot& raw) {
    int acc_dx = 0;
    int acc_dy = 0;
    bool fire = false;

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
                world_->resetBattle();
                state_ = GameState::Battle;
                audit_->write("STATE", "Enter battle.");
            } else if (state_ == GameState::Victory || state_ == GameState::Defeat) {
                state_ = GameState::Title;
                audit_->write("STATE", "Return to title.");
            }
            break;
        case GameCommand::MoveUp:
            acc_dy -= 1;
            break;
        case GameCommand::MoveDown:
            acc_dy += 1;
            break;
        case GameCommand::MoveLeft:
            acc_dx -= 1;
            break;
        case GameCommand::MoveRight:
            acc_dx += 1;
            break;
        case GameCommand::Fire:
            fire = true;
            break;
        }
    }

    domain::PlayerIntent in;
    in.move_dx = (acc_dx < 0) ? -1 : (acc_dx > 0 ? 1 : 0);
    in.move_dy = (acc_dy < 0) ? -1 : (acc_dy > 0 ? 1 : 0);
    in.fire = fire;

    if (state_ == GameState::Battle) {
        const int px_before = world_->player().gx();
        const int py_before = world_->player().gy();

        const float pcx = (static_cast<float>(px_before) + 0.5f) * static_cast<float>(cell_px_);
        const float pcy = (static_cast<float>(py_before) + 0.5f) * static_cast<float>(cell_px_);
        const float dmx = static_cast<float>(raw.mouse_px) - pcx;
        const float dmy = static_cast<float>(raw.mouse_py) - pcy;
        const float len = std::sqrt(dmx * dmx + dmy * dmy);
        if (raw.aim_from_mouse && len > kAimEpsilonPx) {
            in.use_mouse_aim = true;
            in.aim_nx = dmx / len;
            in.aim_ny = dmy / len;
        }

        world_->setIntent(in);
        world_->simulateStep(dt);

        const int px_after = world_->player().gx();
        const int py_after = world_->player().gy();
        const int rdx = px_after - px_before;
        const int rdy = py_after - py_before;
        last_player_move_step_ = (rdx != 0) || (rdy != 0);
        last_player_step_dx_ = static_cast<std::int8_t>(std::max(-1, std::min(1, rdx)));
        last_player_step_dy_ = static_cast<std::int8_t>(std::max(-1, std::min(1, rdy)));

        if (world_->battleOutcome() == domain::BattleOutcome::Victory) {
            state_ = GameState::Victory;
            audit_->write("BATTLE", "Victory.");
        } else if (world_->battleOutcome() == domain::BattleOutcome::Defeat) {
            state_ = GameState::Defeat;
            audit_->write("BATTLE", "Defeat.");
        }
    } else {
        last_player_move_step_ = false;
        last_player_step_dx_ = 0;
        last_player_step_dy_ = 0;
    }
}

RenderSnapshot GameApplication::buildSnapshot() const {
    using domain::ScreenLayout;

    RenderSnapshot s;
    s.total_rows = ScreenLayout::kRows;
    s.total_cols = ScreenLayout::kCols;
    s.sky_rows = ScreenLayout::skyRows();
    s.playfield_rows = ScreenLayout::playfieldRows();
    s.sky_scroll_slow = 0;
    s.sky_scroll_fast = 0;

    const auto& pf = world_->playfield();
    s.playfield_tiles.resize(static_cast<std::size_t>(pf.width() * pf.height()));
    for (int y = 0; y < pf.height(); ++y) {
        for (int x = 0; x < pf.width(); ++x) {
            s.playfield_tiles[static_cast<std::size_t>(y * pf.width() + x)] = pf.tile(x, y);
        }
    }

    s.player_x = world_->player().gx();
    s.player_y = world_->player().gy();
    s.player_hp = world_->player().hp();
    s.player_hp_max = world_->player().maxHp();

    s.enemies.clear();
    for (const auto& e : world_->enemies()) {
        if (!e || e->hp() <= 0) {
            continue;
        }
        EnemyView v;
        v.x = e->gx();
        v.y = e->gy();
        v.kind = e->kind();
        v.hp = e->hp();
        s.enemies.push_back(v);
    }

    s.bullets.clear();
    for (const auto& b : world_->bullets()) {
        if (!b || b->destroyed()) {
            continue;
        }
        BulletView bv;
        bv.world_x = b->x();
        bv.world_y = b->y();
        bv.x = static_cast<int>(std::floor(static_cast<double>(bv.world_x)));
        bv.y = static_cast<int>(std::floor(static_cast<double>(bv.world_y)));
        bv.angle_rad = std::atan2(b->vy(), b->vx());
        bv.rotation_deg =
            std::atan2(-b->vy(), b->vx()) * 180.f / 3.14159265f;
        s.bullets.push_back(bv);
    }

    s.theme = theme_;
    if (state_ == GameState::Victory) {
        s.battle_outcome = BattleOutcomeView::Victory;
    } else if (state_ == GameState::Defeat) {
        s.battle_outcome = BattleOutcomeView::Defeat;
    } else {
        s.battle_outcome = BattleOutcomeView::None;
    }

    if (state_ == GameState::Title) {
        prepareTitleOverlay(s.overlay, s.total_cols, s.total_rows);
    } else if (state_ == GameState::Victory || state_ == GameState::Defeat) {
        prepareBattleEndOverlay(s.overlay, s.total_cols, s.total_rows, state_ == GameState::Victory);
    } else {
        s.overlay.active = false;
        s.overlay.has_start_button = false;
    }

    s.gameplay_active = (state_ == GameState::Battle);
    if (s.gameplay_active) {
        s.player_move_step = last_player_move_step_;
        s.player_step_dx = last_player_step_dx_;
        s.player_step_dy = last_player_step_dy_;
    } else {
        s.player_move_step = false;
        s.player_step_dx = 0;
        s.player_step_dy = 0;
    }

    return s;
}

} // namespace application
