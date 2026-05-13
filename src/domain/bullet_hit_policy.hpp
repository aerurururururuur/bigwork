#pragma once

#include "domain/bullet_faction.hpp"

#include <memory>

namespace domain {

class BulletActor;
class World;

/** Pluggable hit resolution after terrain / bounds checks (player vs enemy bullets). */
class IBulletHitPolicy {
public:
    virtual ~IBulletHitPolicy() = default;
    /** @return true if the bullet should be destroyed after this call. */
    virtual bool resolveActorHits(BulletActor& bullet, World& world) = 0;
};

std::unique_ptr<IBulletHitPolicy> makeBulletHitPolicy(BulletFaction faction);

} // namespace domain
