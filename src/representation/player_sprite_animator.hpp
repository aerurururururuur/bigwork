#pragma once

#include "application/render_snapshot.hpp"
#include "representation/sprite_sheet_config.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace sf {
class Texture;
}

namespace representation {

class PlayerRole2Resources;

/** Layout for drawing Role2 (multi-texture); computed after `update`. */
struct PlayerRole2Draw {
    bool ok{false};
    const sf::Texture* texture{nullptr};
    sf::IntRect rect{};
    float scale_x{1.f};
    float scale_y{1.f};
    float origin_x{0.f};
    float origin_y{0.f};
};

/** Selects idle / run / death frames from a `SpriteSheetConfig` using only `RenderSnapshot`. */
class PlayerSpriteAnimator {
public:
    void set_config(const SpriteSheetConfig* cfg) { cfg_ = cfg; }
    void set_role2_resources(const PlayerRole2Resources* r) { role2_ = r; }

    void update(double dt, const application::RenderSnapshot& snap);

    /** Pixel sub-rectangle for the current frame; empty rect if unusable. */
    sf::IntRect texture_rect(unsigned texture_width, unsigned texture_height) const;

    /** Applies texture rect, origin (feet), and scale: at least slightly above enemy disc (2*pr). */
    void apply_to_sprite(sf::Sprite& sprite, unsigned texture_width, unsigned texture_height, int cell_px,
                          float disc_radius_px) const;

    /** When `snap.player_character` is Role2 and resources valid, fills sprite layout from strip frames. */
    PlayerRole2Draw compute_role2_draw(const application::RenderSnapshot& snap, int cell_px,
                                       float disc_radius_px) const;

private:
    enum class Mode { Idle, Run, Death };

    const SpriteLinearClip& active_clip() const;
    void advance_looping(double dt, const SpriteLinearClip& clip);
    void advance_death(double dt);
    void advance_death_clip(const SpriteLinearClip& clip, double dt);

    const SpriteSheetConfig* cfg_{nullptr};
    const PlayerRole2Resources* role2_{nullptr};
    Mode mode_{Mode::Idle};
    int frame_index_{0};
    double frame_time_{0.0};
    bool death_settled_{false};
    /** Horizontal flip: true when last significant move was left (vx < 0). */
    bool face_left_{false};
    /** -1: use idle/run/death clip; else column 2 rows 1..3 from player sheet (skill overlay). */
    int skill_overlay_frame_{-1};
};

} // namespace representation
