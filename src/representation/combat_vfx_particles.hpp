#pragma once

#include "application/render_snapshot.hpp"

#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

namespace representation {

/** Short-lived bursts from `RenderSnapshot::combat_vfx` (domain events drained into snapshot). */
class CombatVfxParticleSystem {
public:
    CombatVfxParticleSystem();

    void update(double dt, const application::RenderSnapshot& snap, int cell_px);
    void draw(sf::RenderTarget& target) const;

private:
    struct Particle {
        sf::Vector2f pos{};
        sf::Vector2f vel{};
        float age{0.f};
        float life{0.35f};
        float size0{3.f};
        sf::Color rgb{};
    };

    void spawnFromEvents(const application::RenderSnapshot& snap, int cell_px);

    std::vector<Particle> particles_;
    mutable sf::RectangleShape rect_;
    std::mt19937 rng_;
};

} // namespace representation
