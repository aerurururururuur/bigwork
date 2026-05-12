#include "representation/movement_particles.hpp"

#include "representation/render_constants.hpp"

#include <algorithm>
#include <cmath>

namespace representation {

namespace {

constexpr float kGravity = 55.f;
constexpr float kDriftSpeed = 38.f;
constexpr float kLifeMin = 0.32f;
constexpr float kLifeMax = 0.52f;

} // namespace

MovementParticleSystem::MovementParticleSystem() : rng_(std::random_device{}()) {
    rect_.setOutlineThickness(0.f);
}

sf::Color MovementParticleSystem::baseDustColor(application::ThemeStyle theme) {
    if (theme == application::ThemeStyle::ColdNight) {
        return sf::Color(140, 170, 220);
    }
    return sf::Color(220, 190, 120);
}

void MovementParticleSystem::spawnBurst(const application::RenderSnapshot& snap, int cell_px, int count) {
    const float cell_f = static_cast<float>(cell_px);
    const float foot_x = snap.player_world_x * cell_f;
    const float foot_y =
        (static_cast<float>(snap.sky_rows) + snap.player_world_y + 0.42f) * cell_f;

    float bx = -snap.player_vx;
    float by = -snap.player_vy;
    const float blen = std::sqrt(bx * bx + by * by);
    if (blen > 1e-4f) {
        bx /= blen;
        by /= blen;
    } else {
        bx = 0.f;
        by = 1.f;
    }

    std::uniform_real_distribution<float> jx(-14.f, 14.f);
    std::uniform_real_distribution<float> jy(-6.f, 10.f);
    std::uniform_real_distribution<float> life_r(kLifeMin, kLifeMax);
    std::uniform_real_distribution<float> size_r(2.5f, 5.5f);
    std::uniform_int_distribution<int> rgb_j(-22, 22);

    const sf::Color base = baseDustColor(snap.theme);

    for (int i = 0; i < count; ++i) {
        if (particles_.size() >= static_cast<std::size_t>(kMovementParticleCap)) {
            break;
        }
        Particle p{};
        p.pos = sf::Vector2f(foot_x + jx(rng_), foot_y + jy(rng_));
        p.vel = sf::Vector2f(bx * kDriftSpeed + jx(rng_) * 0.35f, by * kDriftSpeed + jy(rng_) * 0.25f);
        p.life = life_r(rng_);
        p.size0 = size_r(rng_);
        p.rgb = sf::Color(static_cast<std::uint8_t>(std::clamp(static_cast<int>(base.r) + rgb_j(rng_), 0, 255)),
                          static_cast<std::uint8_t>(std::clamp(static_cast<int>(base.g) + rgb_j(rng_), 0, 255)),
                          static_cast<std::uint8_t>(std::clamp(static_cast<int>(base.b) + rgb_j(rng_), 0, 255)));
        particles_.push_back(p);
    }
}

void MovementParticleSystem::update(double dt, const application::RenderSnapshot& snap, int cell_px) {
    if (!snap.gameplay_active || snap.overlay.active) {
        particles_.clear();
        dust_emit_acc_ = 0.0;
        return;
    }

    for (auto& p : particles_) {
        p.age += static_cast<float>(dt);
        p.vel.y += kGravity * static_cast<float>(dt);
        p.pos.x += p.vel.x * static_cast<float>(dt);
        p.pos.y += p.vel.y * static_cast<float>(dt);
    }

    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                    [](const Particle& p) { return p.age >= p.life; }),
                     particles_.end());

    if (snap.player_emit_dust) {
        dust_emit_acc_ += dt;
        constexpr double kBurstInterval = 0.055;
        if (dust_emit_acc_ >= kBurstInterval) {
            dust_emit_acc_ = 0.0;
            spawnBurst(snap, cell_px, kMovementParticlesSpawnOnStep);
        }
    } else {
        dust_emit_acc_ = 0.0;
    }
}

void MovementParticleSystem::draw(sf::RenderTarget& target) const {
    for (const Particle& p : particles_) {
        const float t = std::min(1.f, p.age / std::max(1e-4f, p.life));
        const float size = std::max(0.5f, p.size0 * (1.f - t));
        const std::uint8_t a = static_cast<std::uint8_t>(255.f * (1.f - t));
        rect_.setSize(sf::Vector2f(std::floor(size), std::floor(size)));
        rect_.setPosition(std::floor(p.pos.x - size * 0.5f), std::floor(p.pos.y - size * 0.5f));
        rect_.setFillColor(sf::Color(p.rgb.r, p.rgb.g, p.rgb.b, a));
        target.draw(rect_);
    }
}

} // namespace representation
