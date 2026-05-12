#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace application {

enum class ThemeStyle : std::uint8_t { Dusk = 0, ColdNight = 1 };

enum class BattleOutcomeView : std::uint8_t { None = 0, Victory = 1, Defeat = 2 };

enum class BulletFactionView : std::uint8_t { Player = 0, Enemy = 1 };

/** Snapshot-only combat Vfx kinds (mirrors domain CombatVfxKind values). */
enum class CombatVfxKindView : std::uint8_t { EnemyDied = 0, PlayerHitByBullet = 1 };

struct OverlayModel {
    bool active{false};
    std::vector<std::string> lines;
    bool has_start_button{false};
    int start_btn_col{0};
    int start_btn_row{0};
    int start_btn_width{0};
    int start_btn_height{0};
};

struct EnemyView {
    float world_x{0.f};
    float world_y{0.f};
    std::uint8_t archetype{0};
    std::uint8_t sprite_id{0};
    float anim_vx{0.f};
    float anim_vy{0.f};
    int hp{0};
    int hp_max{1};
};

struct BulletView {
    int x{0};
    int y{0};
    float world_x{0.f};
    float world_y{0.f};
    float angle_rad{0.f};
    /** SFML `setRotation`: y-down screen, sprite forward = +x at 0 deg. */
    float rotation_deg{0.f};
    BulletFactionView faction{BulletFactionView::Player};
    /** Domain `EnemyBulletSprite` when `faction == Enemy`; else 0. */
    std::uint8_t enemy_bullet_sprite{0};
};

struct CombatVfxEventView {
    CombatVfxKindView kind{CombatVfxKindView::EnemyDied};
    float world_x{0.f};
    float world_y{0.f};
};

/** Read-only view data for one frame (Representation consumes only this). */
struct RenderSnapshot {
    int total_rows{0};
    int total_cols{0};
    int sky_rows{0};
    int playfield_rows{0};
    int sky_scroll_slow{0};
    int sky_scroll_fast{0};
    std::vector<char> playfield_tiles;
    float player_world_x{0.f};
    float player_world_y{0.f};
    float player_vx{0.f};
    float player_vy{0.f};
    float actor_radius_world{0.35f};
    float elite_melee_manhattan_tiles{4.f};
    int player_hp{0};
    int player_hp_max{0};
    std::vector<EnemyView> enemies;
    std::vector<BulletView> bullets;
    int score{0};
    int combo{0};
    double combo_timer{0.0};
    std::vector<CombatVfxEventView> combat_vfx;
    ThemeStyle theme{ThemeStyle::Dusk};
    BattleOutcomeView battle_outcome{BattleOutcomeView::None};
    OverlayModel overlay;

    /** True only during active battle (for foot-dust etc.); title/victory/defeat false. */
    bool gameplay_active{false};
    /** True when horizontal speed magnitude exceeds epsilon (for foot dust). */
    bool player_emit_dust{false};
};

} // namespace application
