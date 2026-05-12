#include "application/game_application.hpp"

#include "application/overlay_layout.hpp"
#include "domain/bullet_faction.hpp"
#include "domain/combat_events.hpp"
#include "domain/combat_entities.hpp"
#include "domain/enemy_archetype.hpp"
#include "domain/enemy_engagement_constants.hpp"
#include "domain/screen_layout.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kAimEpsilonPx = 2.f;
constexpr float kDustSpeedEpsilon = 0.35f;

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
    bool skill_q = false;

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
        case GameCommand::SkillQ:
            skill_q = true;
            break;
        }
    }

    domain::PlayerIntent in;
    in.move_dx = (acc_dx < 0) ? -1 : (acc_dx > 0 ? 1 : 0);
    in.move_dy = (acc_dy < 0) ? -1 : (acc_dy > 0 ? 1 : 0);
    in.fire = fire;
    in.skill_q = skill_q;

    if (state_ == GameState::Battle) {
        const float pcx = world_->player().x() * static_cast<float>(cell_px_);
        const float pcy = world_->player().y() * static_cast<float>(cell_px_);
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

        if (world_->battleOutcome() == domain::BattleOutcome::Victory) {
            state_ = GameState::Victory;
            audit_->write("BATTLE", "Victory.");
        } else if (world_->battleOutcome() == domain::BattleOutcome::Defeat) {
            state_ = GameState::Defeat;
            audit_->write("BATTLE", "Defeat.");
        }
    }
}

RenderSnapshot GameApplication::buildSnapshot() {
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

    s.player_world_x = world_->player().x();
    s.player_world_y = world_->player().y();
    s.player_vx = world_->player().vx();
    s.player_vy = world_->player().vy();
    s.actor_radius_world = domain::kActorBodyRadius;
    s.player_body_radius_world = domain::kPlayerBodyRadius;
    s.elite_melee_manhattan_tiles = domain::kEliteHybridMeleeManhattanWorld;
    s.player_hp = world_->player().hp();
    s.player_hp_max = world_->player().maxHp();
    s.player_mp = world_->player().mp();
    s.player_mp_max = world_->player().maxMp();
    s.player_skill_anim_remaining = static_cast<float>(world_->player().skillAnimRemaining());
    s.player_skill_anim_total = static_cast<float>(world_->player().skillAnimTotal());

    s.enemies.clear();
    s.battle_has_boss_enemy = false;
    for (const auto& e : world_->enemies()) {
        if (!e || e->hp() <= 0) {
            continue;
        }
        if (e->archetype() == domain::EnemyArchetype::Boss) {
            s.battle_has_boss_enemy = true;
        }
        EnemyView v;
        v.world_x = e->x();
        v.world_y = e->y();
        v.archetype = static_cast<std::uint8_t>(e->archetype());
        v.sprite_id = static_cast<std::uint8_t>(e->spriteId());
        v.anim_vx = e->lastAnimVx();
        v.anim_vy = e->lastAnimVy();
        v.hp = e->hp();
        v.hp_max = std::max(1, e->maxHp());
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
        bv.faction = (b->faction() == domain::BulletFaction::Enemy) ? BulletFactionView::Enemy
                                                                      : BulletFactionView::Player;
        bv.enemy_bullet_sprite = b->enemyBulletVisual();
        s.bullets.push_back(bv);
    }

    s.score = world_->score().totalScore();
    s.combo = world_->score().combo();
    s.combo_timer = world_->score().comboTimer();

    for (const auto& ev : world_->drainCombatVfxEvents()) {
        CombatVfxEventView v;
        v.kind = (ev.kind == domain::CombatVfxKind::PlayerHitByBullet) ? CombatVfxKindView::PlayerHitByBullet
                                                                        : CombatVfxKindView::EnemyDied;
        v.world_x = ev.world_x;
        v.world_y = ev.world_y;
        s.combat_vfx.push_back(v);
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
        const float sp = s.player_vx * s.player_vx + s.player_vy * s.player_vy;
        s.player_emit_dust = sp > kDustSpeedEpsilon * kDustSpeedEpsilon;
    } else {
        s.player_emit_dust = false;
    }

    return s;
}

} // namespace application
