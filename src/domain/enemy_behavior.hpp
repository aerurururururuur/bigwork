#pragma once

#include "domain/combat_ports.hpp"
#include "domain/enemy_archetype.hpp"

#include <memory>

namespace domain {

class EnemyActor;
class World;

struct CombatActorPorts {
    IBulletFirePort* fire{nullptr};
    IMeleeEngagePort* melee{nullptr};
};

class IEnemyBehavior {
public:
    virtual ~IEnemyBehavior() = default;
    virtual void tick(EnemyActor& self, CombatActorPorts ports, World& world, double dt) = 0;
};

std::unique_ptr<IEnemyBehavior> makeEnemyBehavior(EnemyArchetype a);

} // namespace domain
