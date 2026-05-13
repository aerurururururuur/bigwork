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

enum class OverlayPlacement : std::uint8_t { Centered = 0, BottomBar = 1 };

/** Title flow: main menu vs character picker (only when `title_screen` is true). */
enum class TitleUiPhaseView : std::uint8_t { MainMenu = 0, CharacterSelect = 1 };

struct OverlayModel {
    bool active{false};
    std::vector<std::string> lines;
    bool has_start_button{false};
    int start_btn_col{0};
    int start_btn_row{0};
    int start_btn_width{0};
    int start_btn_height{0};
    /** Panel top-left and size in logical cells; filled by overlay_layout prepare*. */
    OverlayPlacement placement{OverlayPlacement::Centered};
    int panel_col{0};
    int panel_row{0};
    int panel_w{0};
    int panel_h{0};
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
    /** Boss-only: hurt strip when true. */
    bool boss_hurt_active{false};
    /** Boss-only: shooting/cast strip when true. */
    bool boss_cast_active{false};
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
    /** Valid when `enemy_bullet_sprite == 2` (BossBullet): strip `0..5` maps to `boss/{1..6}/`. */
    std::uint8_t enemy_bullet_strip{0};
    /** 0 = fallback rect; 1 = Role2 book strip; 2 = Role1 `red.png` bullet. */
    std::uint8_t player_bullet_visual{0};
    /** Sub-frame index for book strip when `player_bullet_visual == 1`. */
    std::uint8_t player_book_subframe{0};
};

struct CombatVfxEventView {
    CombatVfxKindView kind{CombatVfxKindView::EnemyDied};
    float world_x{0.f};
    float world_y{0.f};
};

/** 0 = red heart (HP), 1 = blue heart (MP); mirrors domain::PickupKind. */
struct PickupDropView {
    std::uint8_t kind{0};
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
    /** Enemy / fallback disc radius in world units (matches `kActorBodyRadius`). */
    float actor_radius_world{0.35f};
    /** Player physics / HUD disc radius (matches `kPlayerBodyRadius`, smaller than enemies). */
    float player_body_radius_world{0.26f};
    float elite_melee_manhattan_tiles{4.f};
    int player_hp{0};
    int player_hp_max{0};
    int player_mp{0};
    int player_mp_max{0};
    /** Skill overlay animation; `total <= 0` means idle/run/death clips only. */
    float player_skill_anim_remaining{0.f};
    float player_skill_anim_total{0.f};
    /** `domain::PlayerCharacterId` as uint8 (0 = Role1, 1 = Role2). */
    std::uint8_t player_character{0};
    std::vector<EnemyView> enemies;
    std::vector<BulletView> bullets;
    int score{0};
    int combo{0};
    double combo_timer{0.0};
    /** Player bullet / Q skill damage multiplier from score tiers (1 when not boosted). */
    float player_outgoing_damage_mult{1.f};
    /** Mob wave label for HUD (1..battle_mob_waves_total); during intermission shows upcoming wave. */
    int battle_wave_index{0};
    int battle_mob_waves_total{0};
    int enemies_alive_count{0};
    /** Seconds until next mob wave while intermission active; -1 when not counting down. */
    double next_wave_countdown_sec{-1.0};

    std::vector<PickupDropView> pickups;
    std::vector<CombatVfxEventView> combat_vfx;
    ThemeStyle theme{ThemeStyle::Dusk};
    BattleOutcomeView battle_outcome{BattleOutcomeView::None};
    OverlayModel overlay;

    /** True when battle overlay blocks simulation (bottom story dialog). */
    bool overlay_modal_pause{false};

    /** True when any living enemy uses the Boss archetype (for BGM track selection). */
    bool battle_has_boss_enemy{false};

    /** True during battle without modal story dialog (foot-dust, etc.). */
    bool gameplay_active{false};
    /** True when horizontal speed magnitude exceeds epsilon (for foot dust). */
    bool player_emit_dust{false};

    /** True when `GameState::Title` (for title-only input such as arrow menu nav). */
    bool title_screen{false};
    TitleUiPhaseView title_phase{TitleUiPhaseView::MainMenu};
    std::uint8_t title_main_index{0};
    std::uint8_t title_char_index{0};
};

} // namespace application
