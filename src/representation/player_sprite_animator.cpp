#include "representation/player_sprite_animator.hpp"

#include "representation/player_role2_resources.hpp"

#include <SFML/Graphics/Texture.hpp>

#include <algorithm>
#include <cmath>

namespace representation {

namespace {

constexpr float kRunSpeedEpsilon = 0.35f;

constexpr float kFacingVxEpsilon = 0.02f;

/** Linear scale vs previous tuning (1 = unchanged). */
constexpr float kPlayerVisualSizeMultiplier = 3.2f;

/** Sprite height vs enemy circle diameter (2*pr): keep player visibly a bit larger. */
constexpr float kPlayerHeightOverEnemyDisc = 1.1f;

} // namespace

const SpriteLinearClip& PlayerSpriteAnimator::active_clip() const {
    static const SpriteLinearClip kEmpty{};
    if (!cfg_ || !cfg_->valid) {
        return kEmpty;
    }
    switch (mode_) {
    case Mode::Run:
        return cfg_->run;
    case Mode::Death:
        return cfg_->death;
    case Mode::Idle:
    default:
        return cfg_->idle;
    }
}

void PlayerSpriteAnimator::advance_looping(double dt, const SpriteLinearClip& clip) {
    if (clip.frames.empty() || clip.fps <= 0.f) {
        return;
    }
    frame_time_ += dt;
    const double spf = 1.0 / static_cast<double>(clip.fps);
    const int n = static_cast<int>(clip.frames.size());
    while (frame_time_ >= spf && n > 0) {
        frame_time_ -= spf;
        frame_index_ = (frame_index_ + 1) % n;
    }
}

void PlayerSpriteAnimator::advance_death(double dt) {
    if (!cfg_ || !cfg_->valid) {
        return;
    }
    const SpriteLinearClip& clip = cfg_->death;
    if (clip.frames.empty() || clip.fps <= 0.f) {
        return;
    }
    if (death_settled_) {
        return;
    }
    frame_time_ += dt;
    const double spf = 1.0 / static_cast<double>(clip.fps);
    const int last = static_cast<int>(clip.frames.size()) - 1;
    while (frame_time_ >= spf) {
        frame_time_ -= spf;
        if (frame_index_ >= last) {
            death_settled_ = true;
            frame_index_ = last;
            break;
        }
        ++frame_index_;
    }
}

void PlayerSpriteAnimator::advance_death_clip(const SpriteLinearClip& clip, double dt) {
    if (clip.frames.empty() || clip.fps <= 0.f) {
        return;
    }
    if (death_settled_) {
        return;
    }
    frame_time_ += dt;
    const double spf = 1.0 / static_cast<double>(clip.fps);
    const int last = static_cast<int>(clip.frames.size()) - 1;
    while (frame_time_ >= spf) {
        frame_time_ -= spf;
        if (frame_index_ >= last) {
            death_settled_ = true;
            frame_index_ = last;
            break;
        }
        ++frame_index_;
    }
}

void PlayerSpriteAnimator::update(double dt, const application::RenderSnapshot& snap) {
    const bool use_r2 = role2_ != nullptr && role2_->valid() && snap.player_character == 1u;
    if (!use_r2 && (!cfg_ || !cfg_->valid)) {
        return;
    }

    const bool want_death = snap.battle_outcome == application::BattleOutcomeView::Defeat && snap.player_hp <= 0;

    if (want_death) {
        skill_overlay_frame_ = -1;
        if (mode_ != Mode::Death) {
            mode_ = Mode::Death;
            frame_index_ = 0;
            frame_time_ = 0.0;
            death_settled_ = false;
        }
        if (use_r2) {
            advance_death_clip(role2_->death().clip, dt);
        } else {
            advance_death(dt);
        }
        return;
    }

    if (mode_ == Mode::Death) {
        mode_ = Mode::Idle;
        frame_index_ = 0;
        frame_time_ = 0.0;
        death_settled_ = false;
    }

    if (snap.player_skill_anim_total > 1e-5f && snap.player_skill_anim_remaining > 1e-5f) {
        const float elapsed = snap.player_skill_anim_total - snap.player_skill_anim_remaining;
        const float spf = snap.player_skill_anim_total / 3.f;
        skill_overlay_frame_ =
            std::clamp(static_cast<int>(std::floor(elapsed / std::max(1e-5f, spf))), 0, 2);
    } else {
        skill_overlay_frame_ = -1;
    }

    const float sp = snap.player_vx * snap.player_vx + snap.player_vy * snap.player_vy;
    mode_ = (sp > kRunSpeedEpsilon * kRunSpeedEpsilon) ? Mode::Run : Mode::Idle;
    if (snap.player_vx < -kFacingVxEpsilon) {
        face_left_ = true;
    } else if (snap.player_vx > kFacingVxEpsilon) {
        face_left_ = false;
    }

    if (use_r2 && skill_overlay_frame_ >= 0) {
        // Attack strip index comes from `skill_overlay_frame_`; do not advance idle/run underneath.
    } else if (use_r2) {
        const SpriteLinearClip& clip = (mode_ == Mode::Run) ? role2_->run().clip : role2_->idle().clip;
        advance_looping(dt, clip);
    } else if (cfg_ && cfg_->valid) {
        advance_looping(dt, active_clip());
    }
}

sf::IntRect PlayerSpriteAnimator::texture_rect(unsigned texture_width, unsigned texture_height) const {
    if (!cfg_ || !cfg_->valid || cfg_->columns <= 0 || cfg_->rows <= 0) {
        return {};
    }
    const int fw = static_cast<int>(texture_width) / cfg_->columns;
    const int fh = static_cast<int>(texture_height) / cfg_->rows;
    if (fw <= 0 || fh <= 0) {
        return {};
    }

    if (skill_overlay_frame_ >= 0) {
        constexpr int kSkillCol = 2;
        constexpr int kSkillRow0 = 1;
        const int row = kSkillRow0 + skill_overlay_frame_;
        return sf::IntRect(kSkillCol * fw, row * fh, fw, fh);
    }

    const SpriteLinearClip& clip = active_clip();
    if (clip.frames.empty()) {
        return {};
    }
    int idx = frame_index_;
    if (mode_ == Mode::Death) {
        idx = std::clamp(idx, 0, static_cast<int>(clip.frames.size()) - 1);
        if (death_settled_) {
            idx = static_cast<int>(clip.frames.size()) - 1;
        }
    } else {
        idx = std::clamp(idx, 0, static_cast<int>(clip.frames.size()) - 1);
    }

    const SpriteFrameCell& fc = clip.frames[static_cast<std::size_t>(idx)];
    return sf::IntRect(fc.col * fw, fc.row * fh, fw, fh);
}

void PlayerSpriteAnimator::apply_to_sprite(sf::Sprite& sprite, unsigned texture_width, unsigned texture_height,
                                           int cell_px, float disc_radius_px) const {
    const sf::IntRect rect = texture_rect(texture_width, texture_height);
    if (rect.width <= 0 || rect.height <= 0 || !cfg_ || !cfg_->valid) {
        return;
    }
    sprite.setTextureRect(rect);
    const float frame_h = static_cast<float>(rect.height);
    const float frame_w = static_cast<float>(rect.width);
    const float cell_f = static_cast<float>(std::max(1, cell_px));
    const float from_json = cfg_->scale_cells * cell_f;
    const float from_disc = 2.f * disc_radius_px * kPlayerHeightOverEnemyDisc;
    const float target_h = std::max(from_json, from_disc) * kPlayerVisualSizeMultiplier;
    const float scale = target_h / std::max(1.f, frame_h);
    const float sx = face_left_ ? -scale : scale;
    sprite.setScale(sx, scale);
    sprite.setOrigin(std::floor(frame_w * 0.5f), frame_h);
}

PlayerRole2Draw PlayerSpriteAnimator::compute_role2_draw(const application::RenderSnapshot& snap, int cell_px,
                                                         float disc_radius_px) const {
    PlayerRole2Draw out{};
    if (role2_ == nullptr || !role2_->valid() || snap.player_character != 1u) {
        return out;
    }

    const PlayerRole2Strip* strip = nullptr;
    int idx = 0;
    const bool want_death = snap.battle_outcome == application::BattleOutcomeView::Defeat && snap.player_hp <= 0;
    if (want_death) {
        strip = &role2_->death();
    } else if (skill_overlay_frame_ >= 0) {
        strip = &role2_->attack();
    } else if (mode_ == Mode::Run) {
        strip = &role2_->run();
    } else {
        strip = &role2_->idle();
    }

    if (strip == nullptr || strip->clip.frames.empty() || strip->columns <= 0 || strip->rows <= 0) {
        return out;
    }
    const int n = static_cast<int>(strip->clip.frames.size());
    if (want_death) {
        idx = std::clamp(frame_index_, 0, n - 1);
        if (death_settled_) {
            idx = n - 1;
        }
    } else if (skill_overlay_frame_ >= 0) {
        idx = std::min(skill_overlay_frame_, n - 1);
    } else {
        idx = std::clamp(frame_index_, 0, n - 1);
    }

    const sf::Texture& tex = strip->texture;
    const auto sz = tex.getSize();
    const int fw = static_cast<int>(sz.x) / strip->columns;
    const int fh = static_cast<int>(sz.y) / strip->rows;
    if (fw <= 0 || fh <= 0) {
        return out;
    }
    const SpriteFrameCell& fc = strip->clip.frames[static_cast<std::size_t>(idx)];
    out.rect = sf::IntRect(fc.col * fw, fc.row * fh, fw, fh);
    out.texture = &tex;

    const float frame_h = static_cast<float>(out.rect.height);
    const float frame_w = static_cast<float>(out.rect.width);
    const float cell_f = static_cast<float>(std::max(1, cell_px));
    const float from_json = role2_->scale_cells() * cell_f;
    const float from_disc = 2.f * disc_radius_px * kPlayerHeightOverEnemyDisc;
    const float target_h = std::max(from_json, from_disc) * kPlayerVisualSizeMultiplier;
    const float scale = target_h / std::max(1.f, frame_h);
    out.scale_x = face_left_ ? -scale : scale;
    out.scale_y = scale;
    out.origin_x = std::floor(frame_w * 0.5f);
    out.origin_y = frame_h;
    out.ok = true;
    return out;
}

} // namespace representation
