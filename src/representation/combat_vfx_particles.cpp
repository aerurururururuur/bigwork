#include "representation/combat_vfx_particles.hpp"

#include "representation/render_constants.hpp"

#include <algorithm>
#include <cmath>

namespace representation {

namespace {

constexpr float kBurstSpeed = 95.f;
constexpr float kLifeMin = 0.22f;
constexpr float kLifeMax = 0.48f;

} // namespace

CombatVfxParticleSystem::CombatVfxParticleSystem() : rng_(std::random_device{}()) {
    rect_.setOutlineThickness(0.f);
}

void CombatVfxParticleSystem::spawnFromEvents(const application::RenderSnapshot& snap, int cell_px) {
    const float cell_f = static_cast<float>(cell_px);
    const float sky = static_cast<float>(snap.sky_rows);

    std::uniform_real_distribution<float> ang(-3.14159265f, 3.14159265f);
    std::uniform_real_distribution<float> spd(0.35f, 1.f);
    std::uniform_real_distribution<float> life_r(kLifeMin, kLifeMax);
    std::uniform_real_distribution<float> size_r(2.f, 5.f);
    std::uniform_int_distribution<int> rgb_j(-35, 35);

    for (const auto& ev : snap.combat_vfx) {
        const int count = (ev.kind == application::CombatVfxKindView::EnemyDied) ? 14 : 8;
        const float px = ev.world_x * cell_f;
        const float py = (sky + ev.world_y) * cell_f;

        for (int i = 0; i < count; ++i) {
            if (particles_.size() >= static_cast<std::size_t>(kCombatVfxParticleCap)) {
                return;
            }
            const float a = ang(rng_);
            const float c = std::cos(a);
            const float s = std::sin(a);
            Particle p{};
            p.pos = sf::Vector2f(px, py);
            p.vel = sf::Vector2f(c * kBurstSpeed * spd(rng_), s * kBurstSpeed * spd(rng_));
            p.life = life_r(rng_);
            p.size0 = size_r(rng_);

            sf::Color base(240, 200, 90);
            if (ev.kind == application::CombatVfxKindView::PlayerHitByBullet) {
                base = sf::Color(255, 90, 90);
            } else if (snap.theme == application::ThemeStyle::ColdNight) {
                base = sf::Color(120, 200, 255);
            }

            p.rgb =
                sf::Color(static_cast<std::uint8_t>(std::clamp(static_cast<int>(base.r) + rgb_j(rng_), 0, 255)),
                          static_cast<std::uint8_t>(std::clamp(static_cast<int>(base.g) + rgb_j(rng_), 0, 255)),
                          static_cast<std::uint8_t>(std::clamp(static_cast<int>(base.b) + rgb_j(rng_), 0, 255)));
            particles_.push_back(p);
        }
    }
}

void CombatVfxParticleSystem::update(double dt, const application::RenderSnapshot& snap, int cell_px) {
    if (!snap.gameplay_active || snap.overlay.active) {
        particles_.clear();
        return;
    }

    spawnFromEvents(snap, cell_px);

    constexpr float kDrag = 0.92f;
    for (auto& p : particles_) {
        p.age += static_cast<float>(dt);
        p.vel.x *= kDrag;
        p.vel.y *= kDrag;
        p.vel.y += 28.f * static_cast<float>(dt);
        p.pos.x += p.vel.x * static_cast<float>(dt);
        p.pos.y += p.vel.y * static_cast<float>(dt);
    }

    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                    [](const Particle& p) { return p.age >= p.life; }),
                     particles_.end());
}

void CombatVfxParticleSystem::draw(sf::RenderTarget& target) const {
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
