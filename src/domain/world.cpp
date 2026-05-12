#include "domain/world.hpp"

#include <algorithm>

namespace domain {

World::World(IRandom& rng) : rng_(rng) {
    playfield_ = PlayfieldGrid{};
    const int w = playfield_.width();
    player_.gx_ = w / 2;
    player_.gy_ = playfield_.height() / 2;
    player_.hp_ = player_.max_hp_;
    player_.move_cool_ = 0.0;
    player_.fire_cool_ = 0.0;
    player_.last_fire_dx_ = 1;
    player_.last_fire_dy_ = 0;
}

void World::resetBattle() {
    outcome_ = BattleOutcome::None;
    contact_hurt_cool_ = 0.0;
    intent_ = {};
    frame_intent_ = {};
    bullets_.clear();
    enemies_.clear();
    playfield_ = PlayfieldGrid{};

    const int w = playfield_.width();
    const int h = playfield_.height();
    player_.gx_ = w / 2;
    player_.gy_ = h / 2;
    player_.hp_ = player_.max_hp_;
    player_.move_cool_ = 0.0;
    player_.fire_cool_ = 0.0;
    player_.last_fire_dx_ = 1;
    player_.last_fire_dy_ = 0;

    scatterObstacles();
    spawnEnemies();
}

void World::scatterObstacles() {
    const int n = 10 + rng_.uniformInt(0, 6);
    for (int i = 0; i < n; ++i) {
        for (int t = 0; t < 50; ++t) {
            const int x = rng_.uniformInt(2, playfield_.width() - 3);
            const int y = rng_.uniformInt(2, playfield_.height() - 3);
            if (std::abs(x - player_.gx_) <= 2 && std::abs(y - player_.gy_) <= 2) {
                continue;
            }
            if (playfield_.tileKind(x, y) != TileKind::Floor) {
                continue;
            }
            playfield_.setKind(x, y, TileKind::Obstacle);
            break;
        }
    }
}

void World::spawnEnemies() {
    const int count = 5;
    int placed = 0;
    for (int tries = 0; placed < count && tries < 600; ++tries) {
        const int x = rng_.uniformInt(1, playfield_.width() - 2);
        const int y = rng_.uniformInt(1, playfield_.height() - 2);
        if (!walkableNoActor(x, y)) {
            continue;
        }
        if (enemyAt(x, y)) {
            continue;
        }
        if (std::abs(x - player_.gx_) + std::abs(y - player_.gy_) < 8) {
            continue;
        }
        auto e = std::make_unique<EnemyActor>();
        e->gx_ = x;
        e->gy_ = y;
        e->hp_ = 3;
        e->kind_ = static_cast<std::uint8_t>(placed % 3);
        e->move_cool_ = rng_.uniformInt(0, 10) / 25.0;
        enemies_.push_back(std::move(e));
        ++placed;
    }
}

void World::simulateStep(double dt) {
    if (outcome_ != BattleOutcome::None) {
        return;
    }
    frame_intent_ = intent_;
    intent_ = {};

    player_.step(*this, dt);
    for (auto& e : enemies_) {
        if (e) {
            e->step(*this, dt);
        }
    }
    for (auto& b : bullets_) {
        if (b) {
            b->step(*this, dt);
        }
    }

    cullDynamics();
    applyContactDamage(dt);
    evaluateBattleOutcome();
}

void World::spawnBullet(float x, float y, float vx, float vy, int damage) {
    auto b = std::make_unique<BulletActor>();
    b->x_ = x;
    b->y_ = y;
    b->vx_ = vx;
    b->vy_ = vy;
    b->damage_ = damage;
    b->dead_ = false;
    bullets_.push_back(std::move(b));
}

bool World::walkableNoActor(int gx, int gy) const {
    return playfield_.walkable(gx, gy);
}

bool World::enemyAt(int gx, int gy) const {
    for (const auto& e : enemies_) {
        if (e && e->gx() == gx && e->gy() == gy && e->hp() > 0) {
            return true;
        }
    }
    return false;
}

void World::cullDynamics() {
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
                                 [](const std::unique_ptr<EnemyActor>& p) {
                                     return !p || p->destroyed();
                                 }),
                   enemies_.end());
    bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(),
                                 [](const std::unique_ptr<BulletActor>& p) {
                                     return !p || p->destroyed();
                                 }),
                   bullets_.end());
}

void World::applyContactDamage(double dt) {
    contact_hurt_cool_ -= dt;
    if (contact_hurt_cool_ > 0.0) {
        return;
    }
    for (const auto& e : enemies_) {
        if (!e || e->hp() <= 0) {
            continue;
        }
        if (e->gx() == player_.gx_ && e->gy() == player_.gy_) {
            const int raw = 1;
            const int dmg = std::max(1, raw - player_.armor_);
            player_.hp_ -= dmg;
            contact_hurt_cool_ = 0.6;
            break;
        }
    }
}

void World::evaluateBattleOutcome() {
    if (player_.hp_ <= 0) {
        outcome_ = BattleOutcome::Defeat;
        return;
    }
    bool any = false;
    for (const auto& e : enemies_) {
        if (e && e->hp() > 0) {
            any = true;
            break;
        }
    }
    if (!any) {
        outcome_ = BattleOutcome::Victory;
    }
}

} // namespace domain
