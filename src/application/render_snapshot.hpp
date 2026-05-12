#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace application {

enum class ThemeStyle : std::uint8_t { Dusk = 0, ColdNight = 1 };

enum class BattleOutcomeView : std::uint8_t { None = 0, Victory = 1, Defeat = 2 };

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
    int x{0};
    int y{0};
    std::uint8_t kind{0};
    int hp{0};
};

struct BulletView {
    int x{0};
    int y{0};
    float world_x{0.f};
    float world_y{0.f};
    float angle_rad{0.f};
    /** SFML `setRotation`: y-down screen, sprite forward = +x at 0 deg. */
    float rotation_deg{0.f};
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
    int player_x{0};
    int player_y{0};
    int player_hp{0};
    int player_hp_max{0};
    std::vector<EnemyView> enemies;
    std::vector<BulletView> bullets;
    ThemeStyle theme{ThemeStyle::Dusk};
    BattleOutcomeView battle_outcome{BattleOutcomeView::None};
    OverlayModel overlay;

    /** True only during active battle (for foot-dust etc.); title/victory/defeat false. */
    bool gameplay_active{false};
    /** True if player grid cell changed this simulation tick (discrete "moving"). */
    bool player_move_step{false};
    std::int8_t player_step_dx{0};
    std::int8_t player_step_dy{0};
};

} // namespace application
