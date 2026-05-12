#include "domain/world.hpp"

#include "domain/bullet_faction.hpp"
#include "domain/enemy_archetype.hpp"
#include "domain/enemy_behavior.hpp"
#include "domain/enemy_sprite_id.hpp"
#include "domain/tile_base.hpp"
#include "domain/vec2.hpp"

#include <algorithm>
#include <cmath>

namespace domain {

namespace {

constexpr float kPlayerSpawnInset = 0.5f;
constexpr float kScatterClearRadius = 2.5f;
constexpr float kSpawnEnemyManhattanMin = 8.f;
constexpr int kScatterTriesPerObstacle = 50;
constexpr int kEnemySpawnMaxTries = 600;

constexpr double kPlayerBulletInvulnSec = 0.35;

struct EnemySpawnRow {
    EnemySpriteId sprite;
    EnemyArchetype archetype;
};

constexpr EnemySpawnRow kEnemySpawnTable[] = {
    {EnemySpriteId::Slime, EnemyArchetype::Melee},
    {EnemySpriteId::BugBit, EnemyArchetype::Melee},
    {EnemySpriteId::Spookmoth, EnemyArchetype::Ranged},
    {EnemySpriteId::Pebblin, EnemyArchetype::EliteHybrid},
};

} // namespace

bool World::terrainCircleWalkable(float cx, float cy, float r) const {
    constexpr int kSamples = 8;
    for (int i = 0; i < kSamples; ++i) {
        const float a = (3.14159265358979f * 2.f * static_cast<float>(i)) / static_cast<float>(kSamples);
        const float px = cx + std::cos(a) * r * 0.92f;
        const float py = cy + std::sin(a) * r * 0.92f;
        const int gx = static_cast<int>(std::floor(px));
        const int gy = static_cast<int>(std::floor(py));
        if (!playfield_.inBounds(gx, gy) || !playfield_.walkable(gx, gy)) {
            return false;
        }
    }
    const int cxg = static_cast<int>(std::floor(cx));
    const int cyg = static_cast<int>(std::floor(cy));
    if (!playfield_.inBounds(cxg, cyg) || !playfield_.walkable(cxg, cyg)) {
        return false;
    }
    return true;
}

bool World::overlapsEnemiesCircle(float cx, float cy, float r, const EnemyActor* except) const {
    const float rr = r + kActorBodyRadius;
    const float rr_sq = rr * rr;
    for (const auto& e : enemies_) {
        if (!e || e.get() == except || e->hp() <= 0) {
            continue;
        }
        const float dx = e->x() - cx;
        const float dy = e->y() - cy;
        if (lengthSq(dx, dy) < rr_sq) {
            return true;
        }
    }
    return false;
}

bool World::overlapsPlayerCircle(float cx, float cy, float r) const {
    const float rr = r + kPlayerBodyRadius;
    const float rr_sq = rr * rr;
    const float dx = player_.x_ - cx;
    const float dy = player_.y_ - cy;
    return lengthSq(dx, dy) < rr_sq;
}

bool World::playerFitsAt(float x, float y) const {
    return terrainCircleWalkable(x, y, kPlayerBodyRadius) &&
           !overlapsEnemiesCircle(x, y, kPlayerBodyRadius, nullptr);
}

bool World::enemyFitsAt(float x, float y, const EnemyActor* self) const {
    return terrainCircleWalkable(x, y, kActorBodyRadius) && !overlapsPlayerCircle(x, y, kActorBodyRadius) &&
           !overlapsEnemiesCircle(x, y, kActorBodyRadius, self);
}

World::World(IRandom& rng) : rng_(rng) {
    playfield_ = PlayfieldGrid{};
    const int w = playfield_.width();
    const int h = playfield_.height();
    player_.x_ = static_cast<float>(w) * 0.5f;
    player_.y_ = static_cast<float>(h) * 0.5f;
    player_.hp_ = player_.max_hp_;
    player_.mp_ = player_.max_mp_;
    player_.mp_regen_carry_ = 0.0;
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
    vfx_pending_.clear();
    score_.reset();
    playfield_ = PlayfieldGrid{};
    boss_released_ = false;

    const int w = playfield_.width();
    const int h = playfield_.height();
    player_.x_ = static_cast<float>(w) * 0.5f;
    player_.y_ = static_cast<float>(h) * 0.5f;
    player_.hp_ = player_.max_hp_;
    player_.mp_ = player_.max_mp_;
    player_.mp_regen_carry_ = 0.0;
    player_.fire_cool_ = 0.0;
    player_.last_fire_dx_ = 1;
    player_.last_fire_dy_ = 0;
    player_.bullet_invuln_rem_ = 0.0;
    player_.resetSkillState();

    scatterObstacles();
    spawnEnemies();
}

void World::scatterObstacles() {
    const int n = 10 + rng_.uniformInt(0, 6);
    for (int i = 0; i < n; ++i) {
        for (int t = 0; t < kScatterTriesPerObstacle; ++t) {
            const int x = rng_.uniformInt(2, playfield_.width() - 3);
            const int y = rng_.uniformInt(2, playfield_.height() - 3);
            const float cx = static_cast<float>(x) + kPlayerSpawnInset;
            const float cy = static_cast<float>(y) + kPlayerSpawnInset;
            if (std::hypot(cx - player_.x_, cy - player_.y_) < kScatterClearRadius) {
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
    for (int tries = 0; placed < count && tries < kEnemySpawnMaxTries; ++tries) {
        const int x = rng_.uniformInt(1, playfield_.width() - 2);
        const int y = rng_.uniformInt(1, playfield_.height() - 2);
        if (!playfield_.walkable(x, y)) {
            continue;
        }
        const float ex = static_cast<float>(x) + kPlayerSpawnInset;
        const float ey = static_cast<float>(y) + kPlayerSpawnInset;
        if (std::abs(ex - player_.x_) + std::abs(ey - player_.y_) < kSpawnEnemyManhattanMin) {
            continue;
        }
        if (!enemyFitsAt(ex, ey, nullptr)) {
            continue;
        }
        auto e = std::make_unique<EnemyActor>();
        e->x_ = ex;
        e->y_ = ey;
        const EnemySpawnRow row = kEnemySpawnTable[placed % 4];
        e->resetForSpawn(row.archetype, row.sprite);
        enemies_.push_back(std::move(e));
        ++placed;
    }
}

void World::maybeSpawnBossAfterWaveClear() {
    if (boss_released_) {
        return;
    }
    for (const auto& e : enemies_) {
        if (e && e->hp() > 0) {
            return;
        }
    }

    auto try_spawn = [this](float min_manhattan) -> bool {
        for (int tries = 0; tries < kEnemySpawnMaxTries; ++tries) {
            const int x = rng_.uniformInt(1, playfield_.width() - 2);
            const int y = rng_.uniformInt(1, playfield_.height() - 2);
            if (!playfield_.walkable(x, y)) {
                continue;
            }
            const float ex = static_cast<float>(x) + kPlayerSpawnInset;
            const float ey = static_cast<float>(y) + kPlayerSpawnInset;
            const float md = std::abs(ex - player_.x_) + std::abs(ey - player_.y_);
            if (md < min_manhattan) {
                continue;
            }
            if (!enemyFitsAt(ex, ey, nullptr)) {
                continue;
            }
            auto boss = std::make_unique<EnemyActor>();
            boss->x_ = ex;
            boss->y_ = ey;
            boss->resetForSpawn(EnemyArchetype::Boss, EnemySpriteId::Pebblin);
            enemies_.push_back(std::move(boss));
            return true;
        }
        return false;
    };

    if (try_spawn(kSpawnEnemyManhattanMin)) {
        boss_released_ = true;
        return;
    }
    if (try_spawn(4.f)) {
        boss_released_ = true;
        return;
    }
    for (int y = 1; y < playfield_.height() - 1; ++y) {
        for (int x = 1; x < playfield_.width() - 1; ++x) {
            if (!playfield_.walkable(x, y)) {
                continue;
            }
            const float ex = static_cast<float>(x) + kPlayerSpawnInset;
            const float ey = static_cast<float>(y) + kPlayerSpawnInset;
            if (!enemyFitsAt(ex, ey, nullptr)) {
                continue;
            }
            auto boss = std::make_unique<EnemyActor>();
            boss->x_ = ex;
            boss->y_ = ey;
            boss->resetForSpawn(EnemyArchetype::Boss, EnemySpriteId::Pebblin);
            enemies_.push_back(std::move(boss));
            boss_released_ = true;
            return;
        }
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
    score_.tick(dt);
    maybeSpawnBossAfterWaveClear();
    evaluateBattleOutcome();
}

void World::spawnPlayerBullet(float x, float y, float vx, float vy, int damage) {
    bullets_.push_back(std::make_unique<PlayerBulletActor>(x, y, vx, vy, damage));
}

void World::spawnEnemyBullet(float x, float y, float vx, float vy, int damage, EnemyBulletSprite sprite) {
    bullets_.push_back(std::make_unique<EnemyBulletActor>(x, y, vx, vy, damage, sprite));
}

void World::chasePlayerStep(EnemyActor& self, World& world, double dt, float chase_speed) {
    const float px = world.player().x();
    const float py = world.player().y();
    float mx = px - self.x();
    float my = py - self.y();
    if (lengthSq(mx, my) > kEpsilon * kEpsilon) {
        normalizeOrDefault(mx, my);
    } else {
        mx = 1.f;
        my = 0.f;
    }
    mx *= chase_speed;
    my *= chase_speed;
    self.integrateVelocity(world, mx, my, static_cast<float>(dt));
}

void World::wanderStep(EnemyActor& self, World& world, double dt, float wander_speed,
                       float manhattan_aggro_radius) {
    (void)manhattan_aggro_radius;
    float mx = static_cast<float>(world.random().uniformInt(-1, 1));
    float my = static_cast<float>(world.random().uniformInt(-1, 1));
    if (mx == 0.f && my == 0.f) {
        mx = 1.f;
    }
    normalizeOrDefault(mx, my);
    mx *= wander_speed;
    my *= wander_speed;
    self.integrateVelocity(world, mx, my, static_cast<float>(dt));
}

bool World::lineOfSightClear(float ax, float ay, float bx, float by) const {
    int x0 = static_cast<int>(std::floor(ax));
    int y0 = static_cast<int>(std::floor(ay));
    const int x1 = static_cast<int>(std::floor(bx));
    const int y1 = static_cast<int>(std::floor(by));
    const int dx = std::abs(x1 - x0);
    const int dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int guard = 0;
    for (;;) {
        if (++guard > 512) {
            return false;
        }
        if (!playfield_.walkable(x0, y0)) {
            return false;
        }
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
    return true;
}

void World::onEnemyBulletHitPlayer(int raw_damage) {
    if (player_.bullet_invuln_rem_ > 0.0) {
        return;
    }
    const int dmg = std::max(1, raw_damage - player_.armor_);
    player_.hp_ -= dmg;
    player_.bullet_invuln_rem_ = kPlayerBulletInvulnSec;
    pushCombatVfx(CombatVfxKind::PlayerHitByBullet, player_.x_, player_.y_);
}

void World::notifyEnemyKilled(EnemyActor& enemy) {
    score_.onEnemyKilled(enemy.archetype());
    pushCombatVfx(CombatVfxKind::EnemyDied, enemy.x(), enemy.y());
}

void World::pushCombatVfx(CombatVfxKind kind, float wx, float wy) {
    CombatVfxEvent ev;
    ev.kind = kind;
    ev.world_x = wx;
    ev.world_y = wy;
    vfx_pending_.push_back(ev);
}

std::vector<CombatVfxEvent> World::drainCombatVfxEvents() {
    std::vector<CombatVfxEvent> out;
    out.swap(vfx_pending_);
    return out;
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
    contact_hurt_cool_ = std::max(0.0, contact_hurt_cool_ - dt);
    if (contact_hurt_cool_ > 0.0) {
        return;
    }
    // Separation keeps centers near 2*r, but sliding + timestep often leaves dist_sq > (2r)^2,
    // so add slack or contact damage rarely triggers despite visible proximity.
    constexpr float kMeleeContactSlack = 0.16f;
    const float touch_r = kPlayerBodyRadius + kActorBodyRadius + kMeleeContactSlack;
    const float touch_sq = touch_r * touch_r;
    for (const auto& e : enemies_) {
        if (!e || e->hp() <= 0) {
            continue;
        }
        const float dx = e->x() - player_.x_;
        const float dy = e->y() - player_.y_;
        if (lengthSq(dx, dy) <= touch_sq) {
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
