#pragma once

#include "domain/enemy_bullet_sprite.hpp"

#include <cstdint>

namespace domain {

class EnemyActor;
class World;

/** Spawn bullets (player + enemy); World provides the only implementation. */
class IBulletFirePort {
public:
    virtual ~IBulletFirePort() = default;
    virtual void spawnPlayerBullet(float x, float y, float vx, float vy, int damage,
                                   std::uint8_t player_bullet_visual = 0) = 0;
    virtual void spawnEnemyBullet(float x, float y, float vx, float vy, int damage,
                                   EnemyBulletSprite sprite = EnemyBulletSprite::Generic) = 0;
};

/** Melee chase / wander without spawning bullets. */
class IMeleeEngagePort {
public:
    virtual ~IMeleeEngagePort() = default;
    virtual void chasePlayerStep(EnemyActor& self, World& world, double dt, float chase_speed) = 0;
    virtual void wanderStep(EnemyActor& self, World& world, double dt, float wander_speed,
                            float manhattan_aggro_radius) = 0;
};

} // namespace domain
