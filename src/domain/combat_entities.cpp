#include "domain/combat_entities.hpp"
#include "domain/world.hpp"

#include <cmath>

namespace domain {

bool PlayerActor::destroyed() const noexcept {
    return hp_ <= 0;
}

void PlayerActor::step(World& world, double dt) {
    move_cool_ -= dt;
    fire_cool_ -= dt;

    const PlayerIntent& in = world.frameIntent();

    if (in.move_dx != 0 || in.move_dy != 0) {
        last_fire_dx_ = in.move_dx;
        last_fire_dy_ = in.move_dy;
    }

    if (move_cool_ <= 0.0 && (in.move_dx != 0 || in.move_dy != 0)) {
        const int nx = gx_ + in.move_dx;
        const int ny = gy_ + in.move_dy;
        if (world.walkableNoActor(nx, ny) && !world.enemyAt(nx, ny)) {
            gx_ = nx;
            gy_ = ny;
        }
        move_cool_ = move_period_;
    }

    if (in.fire && fire_cool_ <= 0.0) {
        constexpr float kBulletSpeed = 9.0f;
        float vx = 0.f;
        float vy = 0.f;
        if (in.use_mouse_aim) {
            vx = in.aim_nx * kBulletSpeed;
            vy = in.aim_ny * kBulletSpeed;
        } else {
            int dx = last_fire_dx_;
            int dy = last_fire_dy_;
            if (dx == 0 && dy == 0) {
                dx = 1;
            }
            const float len = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            vx = (static_cast<float>(dx) / len) * kBulletSpeed;
            vy = (static_cast<float>(dy) / len) * kBulletSpeed;
        }
        world.spawnBullet(static_cast<float>(gx_) + 0.5f, static_cast<float>(gy_) + 0.5f, vx, vy, damage_);
        fire_cool_ = fire_period_;
    }
}

bool EnemyActor::destroyed() const noexcept {
    return hp_ <= 0;
}

void EnemyActor::applyDamage(int amount) {
    hp_ -= amount;
    if (hp_ < 0) {
        hp_ = 0;
    }
}

void EnemyActor::step(World& world, double dt) {
    move_cool_ -= dt;
    if (move_cool_ > 0.0) {
        return;
    }

    const int pgx = world.player().gx();
    const int pgy = world.player().gy();
    const int md = std::abs(gx_ - pgx) + std::abs(gy_ - pgy);

    int dx = 0;
    int dy = 0;
    if (md <= 10) {
        if (std::abs(pgx - gx_) >= std::abs(pgy - gy_)) {
            dx = (pgx > gx_) ? 1 : (pgx < gx_) ? -1 : 0;
        } else {
            dy = (pgy > gy_) ? 1 : (pgy < gy_) ? -1 : 0;
        }
    } else {
        dx = world.random().uniformInt(-1, 1);
        dy = world.random().uniformInt(-1, 1);
        if (dx == 0 && dy == 0) {
            dx = 1;
        }
    }

    const int nx = gx_ + dx;
    const int ny = gy_ + dy;
    if (world.walkableNoActor(nx, ny) && !(nx == pgx && ny == pgy)) {
        bool blocked = false;
        for (const auto& o : world.enemies()) {
            if (!o || o.get() == this) {
                continue;
            }
            if (o->hp() > 0 && o->gx() == nx && o->gy() == ny) {
                blocked = true;
                break;
            }
        }
        if (!blocked) {
            gx_ = nx;
            gy_ = ny;
        }
    }
    move_cool_ = move_period_;
}

bool BulletActor::destroyed() const noexcept {
    return dead_;
}

void BulletActor::step(World& world, double dt) {
    if (dead_) {
        return;
    }
    x_ += vx_ * static_cast<float>(dt);
    y_ += vy_ * static_cast<float>(dt);

    const int gx = static_cast<int>(std::floor(x_));
    const int gy = static_cast<int>(std::floor(y_));

    if (!world.walkableNoActor(gx, gy)) {
        dead_ = true;
        return;
    }

    for (auto& e : world.enemies_) {
        if (!e || e->hp() <= 0) {
            continue;
        }
        if (e->gx() == gx && e->gy() == gy) {
            e->applyDamage(std::max(1, damage_));
            dead_ = true;
            return;
        }
    }
}

} // namespace domain
