#include "representation/sfml_game_window.hpp"

#include "application/overlay_layout.hpp"
#include "representation/render_constants.hpp"
#include "representation/sprite_sheet_config.hpp"
#include "representation/theme_palette.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <cstdio>
#include <optional>
#include <string>

namespace {

constexpr const char* kPlayerSheetConfigPath = "assets/sprites/player_sheet.json";

std::uint8_t enemyArchetypeColorId(std::uint8_t archetype) {
    switch (archetype) {
    case 0:
        return 43;
    case 1:
        return 44;
    case 2:
        return 45;
    case 3:
        return 46;
    default:
        return 45;
    }
}

using representation::EnemySheetConfig;
using representation::SpriteLinearClip;

int keyCodeToDevBossSkillSlot(sf::Keyboard::Key code) {
    switch (code) {
    case sf::Keyboard::Num1:
        return 1;
    case sf::Keyboard::Num2:
        return 2;
    case sf::Keyboard::Num3:
        return 3;
    case sf::Keyboard::Num4:
        return 4;
    case sf::Keyboard::Num5:
        return 5;
    case sf::Keyboard::Num6:
        return 6;
    case sf::Keyboard::Num7:
        return 7;
    case sf::Keyboard::Num8:
        return 8;
    case sf::Keyboard::Num9:
        return 9;
    default:
        return 0;
    }
}

int enemy_clip_frame_index(const SpriteLinearClip& c, double t) {
    if (c.frames.empty()) {
        return 0;
    }
    const float fps = std::max(0.01f, c.fps);
    const double spf = 1.0 / static_cast<double>(fps);
    int idx = static_cast<int>(t / spf);
    const int n = static_cast<int>(c.frames.size());
    if (n <= 0) {
        return 0;
    }
    if (c.loop) {
        idx = (idx % n + n) % n;
    } else {
        idx = std::clamp(idx, 0, n - 1);
    }
    return idx;
}

sf::IntRect enemy_sheet_frame_rect(const EnemySheetConfig& cfg, const SpriteLinearClip& clip, int frame_index,
                                     unsigned tex_w, unsigned tex_h) {
    if (cfg.columns <= 0 || cfg.rows <= 0 || clip.frames.empty()) {
        return {};
    }
    const int fw = static_cast<int>(tex_w) / cfg.columns;
    const int fh = static_cast<int>(tex_h) / cfg.rows;
    if (fw <= 0 || fh <= 0) {
        return {};
    }
    frame_index = std::clamp(frame_index, 0, static_cast<int>(clip.frames.size()) - 1);
    const representation::SpriteFrameCell& fc = clip.frames[static_cast<std::size_t>(frame_index)];
    return sf::IntRect(fc.col * fw, fc.row * fh, fw, fh);
}

} // namespace

namespace representation {

SfmlGameWindow::SfmlGameWindow(int logical_cols, int logical_rows, int cell_px,
                               std::optional<std::filesystem::path> background_image_path,
                               bool enable_dev_boss_digit_keys)
    : logical_cols_{logical_cols},
      logical_rows_{logical_rows},
      cell_px_{cell_px},
      enable_dev_boss_digit_keys_{enable_dev_boss_digit_keys} {
    const unsigned width = static_cast<unsigned>(logical_cols * cell_px);
    const unsigned height = static_cast<unsigned>(logical_rows * cell_px);

    sf::ContextSettings ctx;
    ctx.antialiasingLevel = 0;

    window_.create(sf::VideoMode(width, height), "Pixel arena combat", sf::Style::Default, ctx);
    window_.setVerticalSyncEnabled(true);
    window_.setFramerateLimit(60);

    syncLetterboxView();

    cell_rect_.setSize(sf::Vector2f(static_cast<float>(cell_px), static_cast<float>(cell_px)));
    cell_rect_.setOutlineThickness(0.f);

    micro_rect_.setOutlineThickness(0.f);

    overlay_rect_.setOutlineThickness(0.f);

    bullet_shape_.setFillColor(colorFromId(46));
    bullet_shape_.setOutlineThickness(0.f);

    actor_circle_.setPointCount(22);
    actor_circle_.setOutlineThickness(0.f);

    font_ok_ = loadFont();
    if (font_ok_) {
        overlay_text_.setFont(font_);
        overlay_text_.setFillColor(sf::Color(235, 235, 240));
    }

    player_sheet_cfg_ = SpriteSheetConfig::load_from_file(kPlayerSheetConfigPath);
    if (player_sheet_cfg_.valid && player_texture_.loadFromFile(player_sheet_cfg_.texture_path)) {
        const auto sz = player_texture_.getSize();
        const bool grid_ok = (player_sheet_cfg_.columns > 0 && player_sheet_cfg_.rows > 0 &&
                                sz.x % static_cast<unsigned>(player_sheet_cfg_.columns) == 0u &&
                                sz.y % static_cast<unsigned>(player_sheet_cfg_.rows) == 0u);
        if (grid_ok) {
            player_sprite_.setTexture(player_texture_);
            player_anim_.set_config(&player_sheet_cfg_);
            player_sprite_ready_ = true;
        }
    }

    role2_resources_ = PlayerRole2Resources::load_from_file("assets/sprites/player_sheet_role2.json");
    player_anim_.set_role2_resources(&role2_resources_);
    if (player_book_bullet_tex_.loadFromFile("assets/sprites/bullets/Book.png")) {
        player_book_bullet_ready_ = true;
    }
    if (player_role1_bullet_tex_.loadFromFile("assets/sprites/bullets/red.png")) {
        player_role1_bullet_ready_ = true;
    }

    (void)enemy_visuals_.load_from_file("assets/sprites/enemy_visuals.json");

    boss_bullet_frames_.load_from_directory("assets/sprites/bullets/boss");

    boss_cat_resources_ = BossCatResources::load_from_file("assets/sprites/boss/boss1/boss_cat_sheet.json");

    if (background_image_path && !background_image_path->empty()) {
        std::error_code ec;
        if (std::filesystem::exists(*background_image_path, ec)) {
            const std::string path_utf8 = background_image_path->string();
            if (background_texture_.loadFromFile(path_utf8)) {
                background_texture_.setSmooth(true);
                background_sprite_.setTexture(background_texture_);
                const auto ts = background_texture_.getSize();
                if (ts.x > 0u && ts.y > 0u) {
                    const float lw = static_cast<float>(logical_cols * cell_px);
                    const float lh = static_cast<float>(logical_rows * cell_px);
                    background_sprite_.setPosition(0.f, 0.f);
                    background_sprite_.setScale(lw / static_cast<float>(ts.x),
                                               lh / static_cast<float>(ts.y));
                    background_loaded_ = true;
                }
            }
        }
    }
}

bool SfmlGameWindow::loadFont() {
    const std::vector<std::string> paths = {
        "assets/fonts/NotoSansSC-Black.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/lucon.ttf",
    };
    for (const std::string& path : paths) {
        if (font_.loadFromFile(path)) {
            return true;
        }
    }
    return false;
}

bool SfmlGameWindow::isOpen() const { return window_.isOpen(); }

void SfmlGameWindow::syncLetterboxView() {
    const unsigned winW = window_.getSize().x;
    const unsigned winH = window_.getSize().y;
    const float lw = static_cast<float>(logical_cols_ * cell_px_);
    const float lh = static_cast<float>(logical_rows_ * cell_px_);
    if (winW == 0 || winH == 0 || lw <= 0.f || lh <= 0.f) {
        return;
    }
    sf::View view(sf::FloatRect(0.f, 0.f, lw, lh));
    const float wr = static_cast<float>(winW) / lw;
    const float hr = static_cast<float>(winH) / lh;
    const float scale = std::min(wr, hr);
    const float vpW = (lw * scale) / static_cast<float>(winW);
    const float vpH = (lh * scale) / static_cast<float>(winH);
    const float vpL = (1.f - vpW) * 0.5f;
    const float vpT = (1.f - vpH) * 0.5f;
    view.setViewport(sf::FloatRect(vpL, vpT, vpW, vpH));
    window_.setView(view);
}

sf::Vector2f SfmlGameWindow::mapWindowPixelToLogical(sf::RenderWindow& win, sf::Vector2i px) {
    return win.mapPixelToCoords(px);
}

bool SfmlGameWindow::mousePixelInsideClient(const sf::RenderWindow& win, sf::Vector2i px) {
    const auto sz = win.getSize();
    return px.x >= 0 && px.y >= 0 && static_cast<unsigned>(px.x) < sz.x &&
           static_cast<unsigned>(px.y) < sz.y;
}

bool SfmlGameWindow::mouseInputActive() const {
    if (!window_.hasFocus()) {
        return false;
    }
    const sf::Vector2i mp = sf::Mouse::getPosition(window_);
    return mousePixelInsideClient(window_, mp);
}

bool SfmlGameWindow::pollInput(domain::RawInputSnapshot& out, const application::RenderSnapshot& view) {
    syncLetterboxView();

    out = domain::RawInputSnapshot{};
    sf::Event event{};
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            return false;
        }
        if (event.type == sf::Event::Resized) {
            syncLetterboxView();
        }
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                out.cancel = true;
            } else if (event.key.code == sf::Keyboard::Enter) {
                out.confirm = true;
            } else if (event.key.code == sf::Keyboard::T) {
                out.toggle_theme = true;
            } else if (event.key.code == sf::Keyboard::Q && !view.overlay.active) {
                out.skill_q = true;
            } else if (event.key.code == sf::Keyboard::E && !view.overlay.active) {
                out.skill_e = true;
            } else if (event.key.code == sf::Keyboard::Tab && !view.overlay.active) {
                out.toggle_player_character = true;
            } else if (enable_dev_boss_digit_keys_ && event.key.code == sf::Keyboard::X &&
                       !view.overlay.active) {
                out.dev_kill_all_enemies = true;
            } else if (enable_dev_boss_digit_keys_ && !view.overlay.active) {
                const int slot = keyCodeToDevBossSkillSlot(event.key.code);
                if (slot != 0 && out.dev_boss_skill_slot == 0) {
                    out.dev_boss_skill_slot = slot;
                }
            } else if (view.title_screen && view.battle_outcome == application::BattleOutcomeView::None) {
                if (event.key.code == sf::Keyboard::Up) {
                    out.title_menu_nav_up = true;
                } else if (event.key.code == sf::Keyboard::Down) {
                    out.title_menu_nav_down = true;
                }
            }
        }
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            const sf::Vector2i epx(event.mouseButton.x, event.mouseButton.y);
            if (window_.hasFocus() && mousePixelInsideClient(window_, epx)) {
                const sf::Vector2f lp = mapWindowPixelToLogical(window_, epx);
                const float lw = static_cast<float>(logical_cols_ * cell_px_);
                const float lh = static_cast<float>(logical_rows_ * cell_px_);
                if (lp.x >= 0.f && lp.y >= 0.f && lp.x < lw && lp.y < lh) {
                    const int col = static_cast<int>(std::floor(lp.x)) / cell_px_;
                    const int row = static_cast<int>(std::floor(lp.y)) / cell_px_;
                    if (!view.title_screen &&
                        application::overlayStartButtonHit(view.overlay, col, row)) {
                        out.pointer_confirm = true;
                    }
                }
            }
        }
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
        out.up = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
        out.down = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
        out.left = true;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        out.right = true;
    }
    // Do not poll Escape here: `isKeyPressed` is global; if Esc was already down when the
    // window opened, the first frame would set `cancel` and exit immediately. Quit uses
    // `sf::Event::KeyPressed` for Escape above.
    if (!view.overlay.active) {
        if (mouseInputActive()) {
            const sf::Vector2i mp = sf::Mouse::getPosition(window_);
            const sf::Vector2f lp = mapWindowPixelToLogical(window_, mp);
            const float lw = static_cast<float>(logical_cols_ * cell_px_);
            const float lh = static_cast<float>(logical_rows_ * cell_px_);
            if (lp.x >= 0.f && lp.y >= 0.f && lp.x < lw && lp.y < lh) {
                out.mouse_px = static_cast<int>(std::lround(lp.x));
                out.mouse_py = static_cast<int>(std::lround(lp.y));
                out.aim_from_mouse = true;
            }
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            out.fire = true;
        }
        if (mouseInputActive() && sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            out.fire = true;
        }
    }
    return true;
}

void SfmlGameWindow::drawEnemies(const application::RenderSnapshot& snap) {
    const float cell = static_cast<float>(cell_px_);
    const float pr = std::max(2.f, snap.actor_radius_world * cell);
    actor_circle_.setRadius(pr);
    actor_circle_.setOrigin(pr, pr);

    constexpr std::uint8_t kArchetypeEliteHybrid = 2;
    constexpr std::uint8_t kArchetypeBoss = 3;
    constexpr std::uint8_t kSpritePebblin = 3;
    constexpr std::uint8_t kSpriteBossCat = 4;
    const float anim_eps = kEnemyAnimMoveEpsilonWorld;
    const float anim_eps_sq = anim_eps * anim_eps;

    enemy_draw_sprite_.setRotation(0.f);

    for (const auto& e : snap.enemies) {
        const float px = e.world_x * cell;
        const float py = (static_cast<float>(snap.sky_rows) + e.world_y) * cell;

        const int sid = static_cast<int>(e.sprite_id);
        const float speed_sq = e.anim_vx * e.anim_vx + e.anim_vy * e.anim_vy;
        const bool moving = speed_sq > anim_eps_sq;
        const float manhattan = std::fabs(e.world_x - snap.player_world_x) +
                                std::fabs(e.world_y - snap.player_world_y);
        const bool melee_pose = (e.sprite_id == kSpritePebblin && e.archetype == kArchetypeEliteHybrid &&
                                 manhattan <= snap.elite_melee_manhattan_tiles);

        if (e.archetype == kArchetypeBoss && e.sprite_id == kSpriteBossCat && boss_cat_resources_.valid()) {
            const double spec_t = enemy_species_anim_time_[static_cast<std::size_t>(kSpriteBossCat)];
            const BossCatStrip* strip = &boss_cat_resources_.idle();
            if (e.boss_hurt_active) {
                strip = &boss_cat_resources_.hurt();
            } else if (e.boss_cast_active) {
                strip = &boss_cat_resources_.attack();
            } else if (moving) {
                strip = &boss_cat_resources_.run();
            }
            BossCatDraw bd{};
            const bool face_left = e.world_x > snap.player_world_x;
            if (computeBossCatStripDraw(*strip, spec_t, cell_px_, pr, boss_cat_resources_.scale_cells(), face_left,
                                        bd)) {
                enemy_draw_sprite_.setTexture(*bd.texture, true);
                enemy_draw_sprite_.setTextureRect(bd.rect);
                enemy_draw_sprite_.setScale(bd.scale_x, bd.scale_y);
                enemy_draw_sprite_.setOrigin(bd.origin_x, bd.origin_y);
                enemy_draw_sprite_.setPosition(std::floor(px), std::floor(py));
                window_.draw(enemy_draw_sprite_);
            } else {
                actor_circle_.setPosition(std::floor(px), std::floor(py));
                actor_circle_.setFillColor(colorFromId(enemyArchetypeColorId(e.archetype)));
                window_.draw(actor_circle_);
            }
        } else {
            const EnemySheetConfig* cfg = enemy_visuals_.sheet_config(sid);
            const sf::Texture* tex = enemy_visuals_.sheet_texture(sid);
            if (!enemy_visuals_.sprite_ready(sid) || !cfg || !tex || cfg->idle.frames.empty() ||
                cfg->move.frames.empty()) {
                actor_circle_.setPosition(std::floor(px), std::floor(py));
                actor_circle_.setFillColor(colorFromId(enemyArchetypeColorId(e.archetype)));
                window_.draw(actor_circle_);
            } else {
                const SpriteLinearClip* clip = &cfg->move;
                if (melee_pose && cfg->has_melee && !cfg->melee.frames.empty()) {
                    clip = &cfg->melee;
                } else if (!moving) {
                    clip = &cfg->idle;
                }

                const std::size_t sp_idx = static_cast<std::size_t>(sid) < enemy_species_anim_time_.size()
                                               ? static_cast<std::size_t>(sid)
                                               : 0;
                const double spec_t = enemy_species_anim_time_[sp_idx];
                const int fi = enemy_clip_frame_index(*clip, spec_t);
                const sf::IntRect rect =
                    enemy_sheet_frame_rect(*cfg, *clip, fi, tex->getSize().x, tex->getSize().y);
                if (rect.width <= 0 || rect.height <= 0) {
                    actor_circle_.setPosition(std::floor(px), std::floor(py));
                    actor_circle_.setFillColor(colorFromId(enemyArchetypeColorId(e.archetype)));
                    window_.draw(actor_circle_);
                } else {
                    enemy_draw_sprite_.setTexture(*tex, true);
                    enemy_draw_sprite_.setTextureRect(rect);
                    const float frame_h = static_cast<float>(rect.height);
                    const float frame_w = static_cast<float>(rect.width);
                    const float scale_vs = cfg->scale_vs_disc;
                    const float scale = (scale_vs * 2.f * pr) / std::max(1.f, frame_h);
                    const bool flip = e.anim_vx < -anim_eps;
                    enemy_draw_sprite_.setScale(flip ? -scale : scale, scale);
                    enemy_draw_sprite_.setOrigin(std::floor(frame_w * 0.5f), frame_h);
                    enemy_draw_sprite_.setPosition(std::floor(px), std::floor(py));
                    window_.draw(enemy_draw_sprite_);
                }
            }
        }

        if (e.archetype != kArchetypeBoss) {
            const float bar_w = pr * 2.4f;
            const float bar_h = std::max(2.f, cell * 0.11f);
            const float ratio =
                static_cast<float>(e.hp) / static_cast<float>(std::max(1, e.hp_max));
            sf::RectangleShape hp_bg(sf::Vector2f(bar_w, bar_h));
            hp_bg.setOrigin(bar_w * 0.5f, bar_h);
            hp_bg.setPosition(std::floor(px), std::floor(py - pr - 2.f));
            hp_bg.setFillColor(sf::Color(20, 24, 32, 220));
            window_.draw(hp_bg);
            sf::RectangleShape hp_fill(sf::Vector2f(std::max(1.f, bar_w * ratio), bar_h));
            hp_fill.setOrigin(bar_w * 0.5f, bar_h);
            hp_fill.setPosition(std::floor(px), std::floor(py - pr - 2.f));
            hp_fill.setFillColor(colorFromId(42));
            window_.draw(hp_fill);
        }
    }

    const float win_w = static_cast<float>(logical_cols_ * cell_px_);
    for (const auto& e : snap.enemies) {
        if (e.archetype != kArchetypeBoss) {
            continue;
        }
        constexpr float kBossTopBarWidthFrac = 0.58f;
        constexpr float kBossTopBarMarginTop = 14.f;
        const float bar_w = std::max(120.f, win_w * kBossTopBarWidthFrac);
        const float bar_h = std::max(18.f, cell * 0.42f);
        const float cx = win_w * 0.5f;
        const float top_y = kBossTopBarMarginTop + bar_h * 0.5f;
        const float ratio = static_cast<float>(e.hp) / static_cast<float>(std::max(1, e.hp_max));
        sf::RectangleShape hp_bg(sf::Vector2f(bar_w, bar_h));
        hp_bg.setOrigin(bar_w * 0.5f, bar_h * 0.5f);
        hp_bg.setPosition(std::floor(cx), std::floor(top_y));
        hp_bg.setFillColor(sf::Color(20, 24, 32, 235));
        window_.draw(hp_bg);
        sf::RectangleShape hp_fill(sf::Vector2f(std::max(2.f, bar_w * ratio), bar_h));
        hp_fill.setOrigin(bar_w * 0.5f, bar_h * 0.5f);
        hp_fill.setPosition(std::floor(cx), std::floor(top_y));
        hp_fill.setFillColor(colorFromId(42));
        window_.draw(hp_fill);
        break;
    }
}

void SfmlGameWindow::drawPlayer(const application::RenderSnapshot& snap, double dt) {
    const float cell = static_cast<float>(cell_px_);
    const float pr = std::max(2.f, snap.player_body_radius_world * cell);
    const float px = snap.player_world_x * cell;
    const float py = (static_cast<float>(snap.sky_rows) + snap.player_world_y) * cell;
    const float draw_dy = kPlayerSpriteDrawDownCells * static_cast<float>(cell_px_);

    player_anim_.update(dt, snap);
    if (player_sprite_ready_) {
        const PlayerRole2Draw r2 = player_anim_.compute_role2_draw(snap, cell_px_, pr);
        if (r2.ok) {
            player_sprite_.setTexture(*r2.texture, true);
            player_sprite_.setTextureRect(r2.rect);
            player_sprite_.setScale(r2.scale_x, r2.scale_y);
            player_sprite_.setOrigin(r2.origin_x, r2.origin_y);
            const float role2_up = kRole2SpriteDrawUpCells * cell;
            player_sprite_.setPosition(std::floor(px), std::floor(py + draw_dy - role2_up));
            window_.draw(player_sprite_);
        } else {
            player_sprite_.setTexture(player_texture_, false);
            player_anim_.apply_to_sprite(player_sprite_, player_texture_.getSize().x, player_texture_.getSize().y,
                                         cell_px_, pr);
            player_sprite_.setPosition(std::floor(px), std::floor(py + draw_dy));
            window_.draw(player_sprite_);
        }
    } else {
        actor_circle_.setRadius(pr);
        actor_circle_.setOrigin(pr, pr);
        actor_circle_.setPosition(std::floor(px), std::floor(py + draw_dy));
        actor_circle_.setFillColor(colorFromId(47));
        window_.draw(actor_circle_);
    }
}

void SfmlGameWindow::drawPickups(const application::RenderSnapshot& snap) {
    const float cell = static_cast<float>(cell_px_);
    constexpr std::uint8_t kKindBlue = 1;
    const float pr = std::max(3.f, cell * 0.14f);
    actor_circle_.setRadius(pr);
    actor_circle_.setOrigin(pr, pr);
    actor_circle_.setOutlineThickness(std::max(1.f, cell * 0.04f));

    for (const auto& p : snap.pickups) {
        const float px = p.world_x * cell;
        const float py = (static_cast<float>(snap.sky_rows) + p.world_y) * cell;
        actor_circle_.setPosition(std::floor(px), std::floor(py));
        if (p.kind == kKindBlue) {
            actor_circle_.setFillColor(sf::Color(90, 150, 235));
            actor_circle_.setOutlineColor(sf::Color(200, 228, 255, 200));
        } else {
            actor_circle_.setFillColor(sf::Color(215, 58, 68));
            actor_circle_.setOutlineColor(sf::Color(255, 200, 200, 200));
        }
        window_.draw(actor_circle_);
    }
    actor_circle_.setOutlineThickness(0.f);
}

void SfmlGameWindow::drawBullets(const application::RenderSnapshot& snap) {
    const float base_len = std::max(3.f, static_cast<float>(cell_px_) * 0.55f);
    const float base_thick = std::max(2.f, static_cast<float>(cell_px_) * 0.22f);
    bullet_shape_.setOrigin(0.f, 0.f);

    for (std::size_t bullet_i = 0; bullet_i < snap.bullets.size(); ++bullet_i) {
        const auto& b = snap.bullets[bullet_i];
        const bool enemy = (b.faction == application::BulletFactionView::Enemy);
        const float len = enemy ? base_len * 1.15f : base_len;
        const float thick = enemy ? base_thick * 1.35f : base_thick;
        const float px = b.world_x * static_cast<float>(cell_px_);
        const float py = (static_cast<float>(snap.sky_rows) + b.world_y) * static_cast<float>(cell_px_);

        if (enemy && b.enemy_bullet_sprite == kEnemyBulletViewPebblinRock) {
            const sf::Texture* rtex = enemy_visuals_.pebblin_rock_tex();
            if (rtex && rtex->getSize().y > 0u) {
                enemy_draw_sprite_.setTexture(*rtex, true);
                const float tw = static_cast<float>(rtex->getSize().x);
                const float th = static_cast<float>(rtex->getSize().y);
                const float target = thick * 3.2f;
                const float sc = target / th;
                enemy_draw_sprite_.setScale(sc, sc);
                enemy_draw_sprite_.setOrigin(tw * 0.5f, th * 0.5f);
                enemy_draw_sprite_.setPosition(std::floor(px), std::floor(py));
                enemy_draw_sprite_.setRotation(b.rotation_deg);
                window_.draw(enemy_draw_sprite_);
                continue;
            }
        }

        if (enemy && b.enemy_bullet_sprite == kEnemyBulletViewBossBullet &&
            boss_bullet_frames_.strip_ready(b.enemy_bullet_strip)) {
            const int fc = boss_bullet_frames_.frame_count(b.enemy_bullet_strip);
            if (fc > 0) {
                const int base_frame = static_cast<int>(std::floor(book_strip_anim_sec_ * 10.0));
                const int fi = (base_frame + static_cast<int>(bullet_i)) % fc;
                const sf::Texture* btex = boss_bullet_frames_.frame(b.enemy_bullet_strip, fi);
                if (btex && btex->getSize().y > 0u) {
                    enemy_draw_sprite_.setTexture(*btex, true);
                    enemy_draw_sprite_.setTextureRect(
                        sf::IntRect(0, 0, static_cast<int>(btex->getSize().x), static_cast<int>(btex->getSize().y)));
                    const float tw = static_cast<float>(btex->getSize().x);
                    const float th = static_cast<float>(btex->getSize().y);
                    const float target = thick * 3.2f;
                    const float sc = target / th;
                    enemy_draw_sprite_.setScale(sc, sc);
                    enemy_draw_sprite_.setOrigin(tw * 0.5f, th * 0.5f);
                    enemy_draw_sprite_.setPosition(std::floor(px), std::floor(py));
                    enemy_draw_sprite_.setRotation(b.rotation_deg);
                    window_.draw(enemy_draw_sprite_);
                    continue;
                }
            }
        }

        if (!enemy && b.player_bullet_visual == kPlayerBulletViewRole1Red && player_role1_bullet_ready_) {
            const auto tsz = player_role1_bullet_tex_.getSize();
            if (tsz.x > 0u && tsz.y > 0u) {
                enemy_draw_sprite_.setTexture(player_role1_bullet_tex_, true);
                enemy_draw_sprite_.setTextureRect(
                    sf::IntRect(0, 0, static_cast<int>(tsz.x), static_cast<int>(tsz.y)));
                const float tw = static_cast<float>(tsz.x);
                const float th = static_cast<float>(tsz.y);
                const float target = thick * 3.2f;
                const float sc = target / th;
                enemy_draw_sprite_.setScale(sc, sc);
                enemy_draw_sprite_.setOrigin(tw * 0.5f, th * 0.5f);
                enemy_draw_sprite_.setPosition(std::floor(px), std::floor(py));
                enemy_draw_sprite_.setRotation(b.rotation_deg);
                window_.draw(enemy_draw_sprite_);
                continue;
            }
        }

        if (!enemy && b.player_bullet_visual == kPlayerBulletViewRole2Book && player_book_bullet_ready_) {
            const auto bsz = player_book_bullet_tex_.getSize();
            const int cols = kBookBulletStripColumns;
            const int rows = kBookBulletStripRows;
            if (bsz.y > 0u && cols > 0 && rows > 0 && bsz.x >= static_cast<unsigned>(cols) &&
                bsz.y >= static_cast<unsigned>(rows) && (bsz.x % static_cast<unsigned>(cols)) == 0u &&
                (bsz.y % static_cast<unsigned>(rows)) == 0u) {
                const int fw = static_cast<int>(bsz.x / static_cast<unsigned>(cols));
                const int fh = static_cast<int>(bsz.y / static_cast<unsigned>(rows));
                const int anim = static_cast<int>(std::floor(book_strip_anim_sec_ * 10.0)) % cols;
                const int col = (static_cast<int>(b.player_book_subframe) + anim) % cols;
                enemy_draw_sprite_.setTexture(player_book_bullet_tex_, true);
                enemy_draw_sprite_.setTextureRect(sf::IntRect(col * fw, 0, fw, fh));
                const float tw = static_cast<float>(fw);
                const float th = static_cast<float>(fh);
                const float target = thick * 3.2f;
                const float sc = target / th;
                enemy_draw_sprite_.setScale(sc, sc);
                enemy_draw_sprite_.setOrigin(tw * 0.5f, th * 0.5f);
                enemy_draw_sprite_.setPosition(std::floor(px), std::floor(py));
                enemy_draw_sprite_.setRotation(b.rotation_deg);
                window_.draw(enemy_draw_sprite_);
                continue;
            }
        }

        bullet_shape_.setSize(sf::Vector2f(len, thick));
        bullet_shape_.setOrigin(len * 0.5f, thick * 0.5f);
        bullet_shape_.setPosition(std::floor(px), std::floor(py));
        bullet_shape_.setRotation(b.rotation_deg);
        if (enemy) {
            bullet_shape_.setFillColor(colorFromId(39));
            bullet_shape_.setOutlineColor(sf::Color(255, 160, 120, 200));
            bullet_shape_.setOutlineThickness(std::max(0.5f, thick * 0.12f));
        } else {
            bullet_shape_.setFillColor(colorFromId(46));
            bullet_shape_.setOutlineThickness(0.f);
        }
        window_.draw(bullet_shape_);
    }
}

void SfmlGameWindow::drawBattleHud(const application::RenderSnapshot& snap) {
    if (!snap.gameplay_active && !snap.overlay_modal_pause) {
        return;
    }

    const float win_w = static_cast<float>(logical_cols_ * cell_px_);
    constexpr float kHudMargin = 10.f;
    const float bar_w = std::min(220.f, win_w * 0.35f);
    const float bar_h = std::max(3.f, static_cast<float>(cell_px_) * 0.12f);
    const float gap = std::max(2.f, bar_h * 0.35f);
    const float left_x = win_w - kHudMargin - bar_w;
    const float top_y = kHudMargin;

    const float hp_ratio =
        static_cast<float>(snap.player_hp) / static_cast<float>(std::max(1, snap.player_hp_max));
    sf::RectangleShape hp_bg(sf::Vector2f(bar_w, bar_h));
    hp_bg.setPosition(std::floor(left_x), std::floor(top_y));
    hp_bg.setFillColor(sf::Color(20, 24, 32, 220));
    window_.draw(hp_bg);
    sf::RectangleShape hp_fill(sf::Vector2f(std::max(1.f, bar_w * hp_ratio), bar_h));
    hp_fill.setPosition(std::floor(left_x), std::floor(top_y));
    hp_fill.setFillColor(sf::Color(215, 58, 68));
    window_.draw(hp_fill);

    const float mp_y = top_y + bar_h + gap;
    const float mp_ratio =
        static_cast<float>(snap.player_mp) / static_cast<float>(std::max(1, snap.player_mp_max));
    sf::RectangleShape mp_bg(sf::Vector2f(bar_w, bar_h));
    mp_bg.setPosition(std::floor(left_x), std::floor(mp_y));
    mp_bg.setFillColor(sf::Color(20, 24, 32, 220));
    window_.draw(mp_bg);
    sf::RectangleShape mp_fill(sf::Vector2f(std::max(1.f, bar_w * mp_ratio), bar_h));
    mp_fill.setPosition(std::floor(left_x), std::floor(mp_y));
    mp_fill.setFillColor(sf::Color(110, 168, 228));
    window_.draw(mp_fill);

    if (!font_ok_) {
        return;
    }
    const unsigned char_size =
        std::max(8u, static_cast<unsigned>((static_cast<unsigned>(cell_px_) * 38u) / 100u));
    overlay_text_.setCharacterSize(char_size);
    overlay_text_.setFillColor(sf::Color(238, 240, 248));

    const float line_step = static_cast<float>(char_size) * 1.15f;
    float text_y = kHudMargin;

    std::string score_line = "Score: " + std::to_string(snap.score);
    if (snap.combo > 0) {
        const int mult = std::min(8, 1 + snap.combo);
        score_line += "  x" + std::to_string(mult);
    }
    overlay_text_.setString(score_line);
    overlay_text_.setPosition(std::floor(kHudMargin), std::floor(text_y));
    window_.draw(overlay_text_);
    text_y += line_step;

    char dmg_line[64];
    std::snprintf(dmg_line, sizeof(dmg_line), "Damage: x%.2f",
                  static_cast<double>(snap.player_outgoing_damage_mult));
    overlay_text_.setString(dmg_line);
    overlay_text_.setPosition(std::floor(kHudMargin), std::floor(text_y));
    window_.draw(overlay_text_);
    text_y += line_step;

    char wave_line[72];
    std::snprintf(wave_line, sizeof(wave_line), "Wave %d/%d", snap.battle_wave_index,
                  std::max(1, snap.battle_mob_waves_total));
    overlay_text_.setString(wave_line);
    overlay_text_.setPosition(std::floor(kHudMargin), std::floor(text_y));
    window_.draw(overlay_text_);
    text_y += line_step;

    char enemy_line[56];
    std::snprintf(enemy_line, sizeof(enemy_line), "Enemies: %d", snap.enemies_alive_count);
    overlay_text_.setString(enemy_line);
    overlay_text_.setPosition(std::floor(kHudMargin), std::floor(text_y));
    window_.draw(overlay_text_);
    text_y += line_step;

    if (snap.next_wave_countdown_sec >= 0.0) {
        char next_line[56];
        std::snprintf(next_line, sizeof(next_line), "Next: %.1fs", snap.next_wave_countdown_sec);
        overlay_text_.setString(next_line);
        overlay_text_.setPosition(std::floor(kHudMargin), std::floor(text_y));
        window_.draw(overlay_text_);
    }
}

void SfmlGameWindow::drawOverlay(const application::RenderSnapshot& snap) {
    if (!snap.overlay.active || snap.overlay.lines.empty()) {
        return;
    }

    const int ox = snap.overlay.panel_w > 0 ? snap.overlay.panel_col : 0;
    const int oy = snap.overlay.panel_h > 0 ? snap.overlay.panel_row : 0;
    const int box_w = snap.overlay.panel_w > 0 ? snap.overlay.panel_w : 1;
    const int box_h = snap.overlay.panel_h > 0 ? snap.overlay.panel_h : 1;

    const float px = static_cast<float>(ox * cell_px_);
    const float py = static_cast<float>(oy * cell_px_);
    const float pw = static_cast<float>(box_w * cell_px_);
    const float ph = static_cast<float>(box_h * cell_px_);

    const float border_thick = cell_px_ <= 14 ? 1.f : 2.f;

    overlay_rect_.setSize(sf::Vector2f(std::floor(pw), std::floor(ph)));
    overlay_rect_.setPosition(std::floor(px), std::floor(py));
    overlay_rect_.setFillColor(sf::Color(22, 26, 34, 245));
    overlay_rect_.setOutlineColor(sf::Color(188, 194, 210));
    overlay_rect_.setOutlineThickness(border_thick);
    window_.draw(overlay_rect_);

    if (snap.overlay.has_start_button) {
        const float bx = static_cast<float>(snap.overlay.start_btn_col * cell_px_);
        const float by = static_cast<float>(snap.overlay.start_btn_row * cell_px_);
        const float bw = static_cast<float>(snap.overlay.start_btn_width * cell_px_);
        const float bh = static_cast<float>(snap.overlay.start_btn_height * cell_px_);
        overlay_rect_.setSize(sf::Vector2f(std::floor(bw), std::floor(bh)));
        overlay_rect_.setPosition(std::floor(bx), std::floor(by));
        overlay_rect_.setFillColor(colorFromId(9));
        overlay_rect_.setOutlineThickness(0.f);
        window_.draw(overlay_rect_);
    }

    if (!font_ok_) {
        return;
    }

    const unsigned char_size =
        std::max(7u, static_cast<unsigned>((static_cast<unsigned>(cell_px_) * 42u) / 100u));
    overlay_text_.setCharacterSize(char_size);
    overlay_text_.setFillColor(sf::Color(228, 232, 238));

    const float line_step = static_cast<float>(cell_px_);
    const float text_left = std::floor(px + static_cast<float>(cell_px_));
    float line_y = std::floor(py + static_cast<float>(cell_px_) + static_cast<float>(char_size) * 0.12f);

    const bool bottom_bar = snap.overlay.placement == application::OverlayPlacement::BottomBar;
    const std::size_t last_i =
        snap.overlay.lines.empty() ? 0 : snap.overlay.lines.size() - 1;

    for (std::size_t li = 0; li < snap.overlay.lines.size(); ++li) {
        overlay_text_.setString(snap.overlay.lines[li]);
        float tx = std::floor(text_left);
        if (bottom_bar && li == last_i) {
            const float lw = overlay_text_.getLocalBounds().width;
            tx = std::floor(px + pw - static_cast<float>(cell_px_) * 0.55f - lw);
        }
        overlay_text_.setPosition(tx, std::floor(line_y));
        window_.draw(overlay_text_);
        line_y += line_step;
    }
}

void SfmlGameWindow::present(int rows, int cols, const std::vector<domain::FrameCell>& cells,
                               const application::RenderSnapshot& snap, double dt) {
    syncLetterboxView();
    window_.clear(sf::Color(12, 14, 22));
    if (background_loaded_) {
        window_.draw(background_sprite_);
    }
    const std::size_t n = static_cast<std::size_t>(rows * cols);

    const int sky_sub = (kSkyMicroCellsPerAxis > 1 && (cell_px_ % kSkyMicroCellsPerAxis) == 0)
                            ? kSkyMicroCellsPerAxis
                            : 1;
    const int micro_px = cell_px_ / sky_sub;

    cell_rect_.setSize(sf::Vector2f(static_cast<float>(cell_px_), static_cast<float>(cell_px_)));
    micro_rect_.setSize(sf::Vector2f(static_cast<float>(micro_px), static_cast<float>(micro_px)));

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const std::size_t i = static_cast<std::size_t>(r * cols + c);
            if (i >= n || i >= cells.size()) {
                continue;
            }
            const domain::FrameCell& cell = cells[i];
            if (background_loaded_ && cell.ch == '.') {
                continue;
            }
            const int base_x = c * cell_px_;
            const int base_y = r * cell_px_;

            if (r < snap.sky_rows && sky_sub > 1) {
                for (int sy = 0; sy < sky_sub; ++sy) {
                    for (int sx = 0; sx < sky_sub; ++sx) {
                        micro_rect_.setPosition(static_cast<float>(base_x + sx * micro_px),
                                                static_cast<float>(base_y + sy * micro_px));
                        micro_rect_.setFillColor(skyMicroFill(cell.color_id, r, c, sx, sy, sky_sub,
                                                                snap.sky_scroll_slow, snap.sky_scroll_fast));
                        window_.draw(micro_rect_);
                    }
                }
            } else {
                cell_rect_.setPosition(static_cast<float>(base_x), static_cast<float>(base_y));
                cell_rect_.setFillColor(colorFromId(cell.color_id));
                window_.draw(cell_rect_);
            }
        }
    }

    movement_particles_.update(dt, snap, cell_px_);
    movement_particles_.draw(window_);

    combat_vfx_.update(dt, snap, cell_px_);
    combat_vfx_.draw(window_);

    for (double& t : enemy_species_anim_time_) {
        t += dt;
    }
    book_strip_anim_sec_ += dt;

    drawEnemies(snap);
    drawBullets(snap);
    drawPlayer(snap, dt);
    drawPickups(snap);
    drawBattleHud(snap);
    drawOverlay(snap);
    window_.display();
}

} // namespace representation
