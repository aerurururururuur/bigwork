#pragma once

#include "application/render_snapshot.hpp"
#include "domain/ports/iterminal.hpp"
#include "domain/raw_input.hpp"
#include "representation/combat_vfx_particles.hpp"
#include "representation/enemy_visual_resources.hpp"
#include "representation/movement_particles.hpp"
#include "representation/player_sprite_animator.hpp"
#include "representation/sprite_sheet_config.hpp"

#include <SFML/Graphics.hpp>

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace representation {

/** Standalone SFML window: polls input and presents a scaled logical pixel grid (solid fills). */
class SfmlGameWindow {
public:
    /** @param background_image_path optional PNG/JPG; stretched to full logical client area. */
    /** @param enable_dev_boss_digit_keys when true, Num1-9 edge map to `dev_boss_skill_slot` in battle UI. */
    SfmlGameWindow(int logical_cols, int logical_rows, int cell_px,
                   std::optional<std::filesystem::path> background_image_path = std::nullopt,
                   bool enable_dev_boss_digit_keys = false);

    bool isOpen() const;
    /** Returns false when the user closed the window. */
    bool pollInput(domain::RawInputSnapshot& out, const application::RenderSnapshot& view);
    void present(int rows, int cols, const std::vector<domain::FrameCell>& cells,
                   const application::RenderSnapshot& snap, double dt);

    bool customBackgroundReady() const noexcept { return background_loaded_; }

private:
    bool loadFont();
    void syncLetterboxView();
    static sf::Vector2f mapWindowPixelToLogical(sf::RenderWindow& win, sf::Vector2i px);
    static bool mousePixelInsideClient(const sf::RenderWindow& win, sf::Vector2i px);
    bool mouseInputActive() const;

    void drawOverlay(const application::RenderSnapshot& snap);
    void drawEnemies(const application::RenderSnapshot& snap);
    void drawBullets(const application::RenderSnapshot& snap);
    void drawPlayer(const application::RenderSnapshot& snap, double dt);
    void drawBattleHud(const application::RenderSnapshot& snap);

    MovementParticleSystem movement_particles_;
    CombatVfxParticleSystem combat_vfx_;

    sf::RenderWindow window_;
    sf::Font font_;
    bool font_ok_{false};
    int logical_cols_{0};
    int logical_rows_{0};
    int cell_px_{16};
    sf::RectangleShape cell_rect_;
    sf::RectangleShape micro_rect_;
    sf::RectangleShape overlay_rect_;
    sf::RectangleShape bullet_shape_;
    sf::CircleShape actor_circle_{32};
    sf::Text overlay_text_;

    SpriteSheetConfig player_sheet_cfg_{};
    sf::Texture player_texture_{};
    sf::Sprite player_sprite_{};
    PlayerSpriteAnimator player_anim_{};
    bool player_sprite_ready_{false};

    EnemyVisualResources enemy_visuals_;
    sf::Sprite enemy_draw_sprite_{};
    /** Per `EnemySpriteId` species: looping animation clock for sheet clips. */
    std::array<double, 4> enemy_species_anim_time_{};

    sf::Texture background_texture_{};
    sf::Sprite background_sprite_{};
    bool background_loaded_{false};
    bool enable_dev_boss_digit_keys_{false};
};

} // namespace representation
