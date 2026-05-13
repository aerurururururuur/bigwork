#include "application/game_application.hpp"

#include "application/overlay_layout.hpp"
#include "domain/bullet_faction.hpp"
#include "domain/combat_events.hpp"
#include "domain/combat_entities.hpp"
#include "domain/enemy_archetype.hpp"
#include "domain/enemy_engagement_constants.hpp"
#include "domain/screen_layout.hpp"
#include "domain/skill.hpp"
#include "infrastructure/game_config.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kAimEpsilonPx = 2.f;
constexpr float kDustSpeedEpsilon = 0.35f;
/** Upper bound for `RawInputSnapshot::dev_boss_skill_slot` (keys 1-9). */
constexpr int kBossDevDigitSlotMax = 9;

bool worldHasLivingBoss(const domain::World& world) {
    for (const auto& e : world.enemies()) {
        if (!e || e->hp() <= 0) {
            continue;
        }
        if (e->archetype() == domain::EnemyArchetype::Boss) {
            return true;
        }
    }
    return false;
}

void dispatchDevKillAllEnemies(domain::World& world, infrastructure::RunMode run_mode,
                               const domain::RawInputSnapshot& raw) {
    if (run_mode != infrastructure::RunMode::Development || !raw.dev_kill_all_enemies) {
        return;
    }
    world.devKillAllEnemies();
}

void dispatchDevBossManualSkill(domain::World& world, infrastructure::RunMode run_mode,
                                const domain::RawInputSnapshot& raw) {
    if (run_mode != infrastructure::RunMode::Development) {
        return;
    }
    const int k = raw.dev_boss_skill_slot;
    if (k <= 0 || k > kBossDevDigitSlotMax || k > domain::kBossManualSkillHotkeyCount) {
        return;
    }
    domain::EnemyActor* boss = nullptr;
    for (const auto& e : world.enemies()) {
        if (!e || e->destroyed() || e->hp() <= 0) {
            continue;
        }
        if (e->archetype() == domain::EnemyArchetype::Boss) {
            boss = e.get();
            break;
        }
    }
    if (boss == nullptr) {
        return;
    }
    domain::SkillCastContext ctx{world, domain::SkillCasterKind::Boss, nullptr, boss, 0.f, 0.f, 0, 1};
    switch (k) {
    case 1:
        domain::bossDiffusionFireRing1(ctx);
        break;
    case 2:
        domain::bossDiffusionFireRing2(ctx);
        break;
    case 3:
        domain::bossFanBarrageFire(ctx);
        break;
    case 4:
        domain::bossSpiralBurstFire(ctx);
        break;
    case 5:
        domain::bossDualOpposingFanFire(ctx);
        break;
    case 6:
        domain::bossCrossDualRingFire(ctx);
        break;
    case 7:
        domain::bossSoftScatterFire(ctx);
        break;
    case 8:
        domain::bossSideWallVolleyFire(ctx);
        break;
    default:
        break;
    }
}

} // namespace

namespace application {

GameApplication::GameApplication(domain::World& world, domain::IAuditSink& audit, int logical_cell_px,
                                   const infrastructure::GameConfig& cfg)
    : world_{&world},
      audit_{&audit},
      cell_px_{logical_cell_px > 0 ? logical_cell_px : 16},
      run_mode_{cfg.run_mode} {
    domain::PlayerDamageScoreBrackets br{};
    br.tier1_score = cfg.player_damage_score_tier1;
    br.tier1_mult = cfg.player_damage_mult_tier1;
    br.tier2_score = cfg.player_damage_score_tier2;
    br.tier2_mult = cfg.player_damage_mult_tier2;
    world_->setPlayerDamageScoreBrackets(br);
    world_->setWaveRuntimeConfig(cfg.wave);
    audit_->write("SESSION", "GameApplication constructed (pixel combat MVP).");
}

void GameApplication::enqueueDialog(std::vector<std::string> lines) {
    dialog_queue_.push_back(std::move(lines));
}

void GameApplication::tick(double dt, const std::vector<GameCommand>& commands,
                           const domain::RawInputSnapshot& raw) {
    for (GameCommand cmd : commands) {
        if (cmd == GameCommand::Quit) {
            audit_->write("INPUT", "Quit requested.");
            quit_requested_ = true;
        } else if (cmd == GameCommand::ToggleTheme) {
            theme_ = (theme_ == ThemeStyle::Dusk) ? ThemeStyle::ColdNight : ThemeStyle::Dusk;
            audit_->write("THEME", theme_ == ThemeStyle::Dusk ? "dusk" : "cold_night");
        }
    }

    bool dialog_blocking = state_ == GameState::Battle && !dialog_queue_.empty();
    if (dialog_blocking) {
        for (GameCommand cmd : commands) {
            if (cmd == GameCommand::Confirm) {
                dialog_queue_.pop_front();
                audit_->write("DIALOG", "story segment dismissed");
                break;
            }
        }
    }

    dialog_blocking = state_ == GameState::Battle && !dialog_queue_.empty();
    if (!dialog_blocking) {
        if (state_ == GameState::Title) {
            if (raw.title_menu_nav_up) {
                if (title_ui_phase_ == TitleUiPhaseView::CharacterSelect) {
                    title_char_index_ = (title_char_index_ <= 0) ? 0 : title_char_index_ - 1;
                } else {
                    title_main_index_ = (title_main_index_ <= 0) ? 0 : title_main_index_ - 1;
                }
            }
            if (raw.title_menu_nav_down) {
                if (title_ui_phase_ == TitleUiPhaseView::CharacterSelect) {
                    title_char_index_ = (title_char_index_ >= 1) ? 1 : title_char_index_ + 1;
                } else {
                    title_main_index_ = (title_main_index_ >= 1) ? 1 : title_main_index_ + 1;
                }
            }
            bool pressed_confirm = raw.confirm;
            if (!pressed_confirm) {
                for (GameCommand c : commands) {
                    if (c == GameCommand::Confirm) {
                        pressed_confirm = true;
                        break;
                    }
                }
            }
            if (pressed_confirm) {
                if (title_ui_phase_ == TitleUiPhaseView::CharacterSelect) {
                    world_->setPlayerCharacter(title_char_index_ == 0 ? domain::PlayerCharacterId::Role1
                                                                      : domain::PlayerCharacterId::Role2);
                    title_ui_phase_ = TitleUiPhaseView::MainMenu;
                    audit_->write("STATE", "Title character selected.");
                } else if (title_main_index_ == 1) {
                    title_ui_phase_ = TitleUiPhaseView::CharacterSelect;
                    title_char_index_ =
                        (world_->player().characterId() == domain::PlayerCharacterId::Role2) ? 1 : 0;
                } else {
                    world_->resetBattle();
                    dialog_queue_.clear();
                    had_living_boss_ = false;
                    boss_intro_dialog_fired_ = false;
                    state_ = GameState::Battle;
                    audit_->write("STATE", "Enter battle.");
                }
            }
        } else {
            for (GameCommand cmd : commands) {
                if (cmd != GameCommand::Confirm) {
                    continue;
                }
                if (state_ == GameState::Victory || state_ == GameState::Defeat) {
                    state_ = GameState::Title;
                    title_ui_phase_ = TitleUiPhaseView::MainMenu;
                    title_main_index_ = 0;
                    audit_->write("STATE", "Return to title.");
                }
            }
        }
    }

    int acc_dx = 0;
    int acc_dy = 0;
    bool fire = false;
    bool skill_q = false;
    bool skill_e = false;

    if (state_ == GameState::Battle && dialog_queue_.empty()) {
        for (GameCommand cmd : commands) {
            switch (cmd) {
            case GameCommand::None:
                break;
            case GameCommand::Quit:
            case GameCommand::ToggleTheme:
            case GameCommand::Confirm:
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
            case GameCommand::SkillE:
                skill_e = true;
                break;
            case GameCommand::SwapPlayerCharacter:
                world_->cyclePlayerCharacter();
                break;
            }
        }
    }

    domain::PlayerIntent in;
    in.move_dx = (acc_dx < 0) ? -1 : (acc_dx > 0 ? 1 : 0);
    in.move_dy = (acc_dy < 0) ? -1 : (acc_dy > 0 ? 1 : 0);
    in.fire = fire;
    in.skill_q = skill_q;
    in.skill_e = skill_e;

    if (state_ == GameState::Battle && dialog_queue_.empty()) {
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
        dispatchDevKillAllEnemies(*world_, run_mode_, raw);
        dispatchDevBossManualSkill(*world_, run_mode_, raw);
        world_->simulateStep(dt);

        if (world_->battleOutcome() == domain::BattleOutcome::Victory) {
            dialog_queue_.clear();
            state_ = GameState::Victory;
            audit_->write("BATTLE", "Victory.");
        } else if (world_->battleOutcome() == domain::BattleOutcome::Defeat) {
            dialog_queue_.clear();
            state_ = GameState::Defeat;
            audit_->write("BATTLE", "Defeat.");
        } else {
            const bool has_boss = worldHasLivingBoss(*world_);
            if (!had_living_boss_ && has_boss && !boss_intro_dialog_fired_) {
                enqueueDialog(std::vector<std::string>{"Meow?"});
                boss_intro_dialog_fired_ = true;
                audit_->write("DIALOG", "boss intro queued");
            }
            had_living_boss_ = has_boss;
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
    s.player_character = static_cast<std::uint8_t>(world_->player().characterId());

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
        if (e->archetype() == domain::EnemyArchetype::Boss) {
            v.boss_hurt_active = e->bossHurtAnimRem() > 0.0;
            v.boss_cast_active = e->bossCastAnimRem() > 0.0;
        }
        s.enemies.push_back(v);
    }

    s.bullets.clear();
    int book_bullet_seq = 0;
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
        bv.enemy_bullet_strip = b->enemyBulletStrip();
        bv.player_bullet_visual = b->playerBulletVisual();
        if (bv.player_bullet_visual == 1) {
            bv.player_book_subframe = static_cast<std::uint8_t>(book_bullet_seq % 10);
            ++book_bullet_seq;
        }
        s.bullets.push_back(bv);
    }

    s.pickups.clear();
    for (const auto& p : world_->pickups()) {
        PickupDropView v;
        v.kind = static_cast<std::uint8_t>(p.kind);
        v.world_x = p.x;
        v.world_y = p.y;
        s.pickups.push_back(v);
    }

    s.score = world_->score().totalScore();
    s.combo = world_->score().combo();
    s.combo_timer = world_->score().comboTimer();
    s.player_outgoing_damage_mult = world_->playerOutgoingDamageMultiplier();

    s.battle_wave_index = world_->battleHudMobWave();
    s.battle_mob_waves_total = world_->mobWavesTotal();
    s.enemies_alive_count = world_->enemiesAliveCount();
    const double inter = world_->waveIntermissionRemaining();
    s.next_wave_countdown_sec = inter > 0.0 ? inter : -1.0;

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
        s.title_screen = true;
        s.title_phase = title_ui_phase_;
        s.title_main_index = static_cast<std::uint8_t>(title_main_index_);
        s.title_char_index = static_cast<std::uint8_t>(title_char_index_);
        if (title_ui_phase_ == TitleUiPhaseView::CharacterSelect) {
            prepareTitleCharacterSelectOverlay(s.overlay, s.total_cols, s.total_rows, title_char_index_);
        } else {
            prepareTitleMainMenuOverlay(s.overlay, s.total_cols, s.total_rows, title_main_index_);
        }
    } else if (state_ == GameState::Victory || state_ == GameState::Defeat) {
        prepareBattleEndOverlay(s.overlay, s.total_cols, s.total_rows, state_ == GameState::Victory);
    } else if (state_ == GameState::Battle && !dialog_queue_.empty()) {
        prepareBottomDialogOverlay(s.overlay, s.total_cols, s.total_rows, dialog_queue_.front());
    } else {
        s.overlay.active = false;
        s.overlay.has_start_button = false;
        s.overlay.panel_col = 0;
        s.overlay.panel_row = 0;
        s.overlay.panel_w = 0;
        s.overlay.panel_h = 0;
        s.overlay.placement = OverlayPlacement::Centered;
    }

    const bool battle_dialog = state_ == GameState::Battle && !dialog_queue_.empty();
    s.overlay_modal_pause = battle_dialog;
    s.gameplay_active = (state_ == GameState::Battle && !battle_dialog);
    if (s.gameplay_active) {
        const float sp = s.player_vx * s.player_vx + s.player_vy * s.player_vy;
        s.player_emit_dust = sp > kDustSpeedEpsilon * kDustSpeedEpsilon;
    } else {
        s.player_emit_dust = false;
    }

    return s;
}

} // namespace application
