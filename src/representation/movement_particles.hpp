#pragma once

#include "application/render_snapshot.hpp"

#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

namespace representation {

/** Purely visual foot dust; consumes only `RenderSnapshot` movement flags. */
class MovementParticleSystem {
public:
    MovementParticleSystem();

    void update(double dt, const application::RenderSnapshot& snap, int cell_px);
    void draw(sf::RenderTarget& target) const;

private:
    struct Particle {
        sf::Vector2f pos{};
        sf::Vector2f vel{};
        float age{0.f};
        float life{0.45f};
        float size0{4.f};
        sf::Color rgb{};
    };

    void spawnBurst(const application::RenderSnapshot& snap, int cell_px, int count);
    static sf::Color baseDustColor(application::ThemeStyle theme);

    std::vector<Particle> particles_;
    mutable sf::RectangleShape rect_;
    std::mt19937 rng_;
    double dust_emit_acc_{0.0};
};

} // namespace representation
