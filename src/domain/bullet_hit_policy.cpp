#include "domain/bullet_hit_policy.hpp"

#include "domain/combat_entities.hpp"
#include "domain/vec2.hpp"
#include "domain/world.hpp"

#include <algorithm>

namespace domain {

namespace {

constexpr float kHitREnemyForPlayerBullet = kBulletBodyRadius + kEnemyPlayerBulletHitRadius;
constexpr float kHitREnemyForPlayerBulletSq = kHitREnemyForPlayerBullet * kHitREnemyForPlayerBullet;

constexpr float kHitRPlayerForEnemyBullet = kBulletBodyRadius + kPlayerBodyRadius;
constexpr float kHitRPlayerForEnemyBulletSq = kHitRPlayerForEnemyBullet * kHitRPlayerForEnemyBullet;

} // namespace

class PlayerBulletHitPolicy final : public IBulletHitPolicy {
public:
    bool resolveActorHits(BulletActor& bullet, World& world) override;
};

class EnemyBulletHitPolicy final : public IBulletHitPolicy {
public:
    bool resolveActorHits(BulletActor& bullet, World& world) override;
};

bool PlayerBulletHitPolicy::resolveActorHits(BulletActor& bullet, World& world) {
    for (auto& e : world.enemies()) {
        if (!e || e->hp() <= 0) {
            continue;
        }
        const float dx = e->x() - bullet.x();
        const float dy = e->y() - bullet.y();
        if (lengthSq(dx, dy) < kHitREnemyForPlayerBulletSq) {
            e->applyDamage(std::max(1, bullet.damage()), &world);
            return true;
        }
    }
    return false;
}

bool EnemyBulletHitPolicy::resolveActorHits(BulletActor& bullet, World& world) {
    const float dx = world.player().x() - bullet.x();
    const float dy = world.player().y() - bullet.y();
    if (lengthSq(dx, dy) < kHitRPlayerForEnemyBulletSq) {
        world.onEnemyBulletHitPlayer(std::max(1, bullet.damage()));
        return true;
    }
    return false;
}

std::unique_ptr<IBulletHitPolicy> makeBulletHitPolicy(BulletFaction faction) {
    switch (faction) {
    case BulletFaction::Player:
        return std::make_unique<PlayerBulletHitPolicy>();
    case BulletFaction::Enemy:
    default:
        return std::make_unique<EnemyBulletHitPolicy>();
    }
}

} // namespace domain
