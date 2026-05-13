#include "domain/world.hpp"

#include "domain/bullet_faction.hpp"
#include "domain/enemy_archetype.hpp"
#include "domain/enemy_behavior.hpp"
#include "domain/enemy_sprite_id.hpp"
#include "domain/tile_base.hpp"
#include "domain/vec2.hpp"
#include "domain/pickup_drop_tuning.hpp"
#include "domain/wave_combat_tuning.hpp"

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
constexpr float kBossSpawnAboveCenterTiles = 2.f;

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

constexpr float kBossPhaseAddSpawnManhattanMin = 5.f;

bool livingBossPresent(const std::vector<std::unique_ptr<EnemyActor>>& enemies) {
    for (const auto& e : enemies) {
        if (e && e->hp() > 0 && e->archetype() == EnemyArchetype::Boss) {
            return true;
        }
    }
    return false;
}

int livingBossMinionCount(const std::vector<std::unique_ptr<EnemyActor>>& enemies) {
    int n = 0;
    for (const auto& e : enemies) {
        if (e && e->hp() > 0 && e->archetype() == EnemyArchetype::BossMinion) {
            ++n;
        }
    }
    return n;
}

EnemySpawnRow pickSpawnRowForWave(int placed, int wave_number, IRandom& rng) {
    EnemySpawnRow row = kEnemySpawnTable[(placed + std::max(1, wave_number) - 1) % 4];
    if (wave_number >= 2 && placed == 0) {
        row = {EnemySpriteId::Spookmoth, EnemyArchetype::Ranged};
    }
    if (wave_number >= 3 && rng.uniformInt(0, 9) < 3) {
        row = kEnemySpawnTable[3];
    }
    return row;
}

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
    pickups_.clear();
    vfx_pending_.clear();
    score_.reset();
    playfield_ = PlayfieldGrid{};
    boss_released_ = false;
    mob_wave_index_ = 1;
    wave_intermission_rem_ = 0.0;
    boss_add_spawn_rem_ = 0.0;
    player_bullet_max_travel_world_ = wave_combat::kPlayerMobBulletTravelWorld;

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
    spawnEnemiesForWave(1);
}

void World::scatterObstacles() {
    const int n = wave_cfg_.scatter_obstacle_base + rng_.uniformInt(0, wave_cfg_.scatter_obstacle_extra_roll);
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

void World::spawnEnemiesForWave(int wave_number) {
    const int wn = std::max(1, wave_number);
    const int count = std::min(wave_cfg_.mob_spawn_max_count,
                                 wave_cfg_.mob_spawn_base_count + (wn - 1) * wave_cfg_.mob_spawn_per_wave_extra);
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
        const EnemySpawnRow row = pickSpawnRowForWave(placed, wn, rng_);
        e->resetForSpawn(row.archetype, row.sprite);
        enemies_.push_back(std::move(e));
        ++placed;
    }
}

void World::maybeSpawnBossAfterWaveClear(double dt) {
    if (boss_released_) {
        return;
    }
    for (const auto& e : enemies_) {
        if (e && e->hp() > 0 && e->archetype() == EnemyArchetype::Boss) {
            return;
        }
    }

    int mob_alive = 0;
    for (const auto& e : enemies_) {
        if (!e || e->hp() <= 0) {
            continue;
        }
        if (e->archetype() != EnemyArchetype::Boss) {
            ++mob_alive;
        }
    }
    if (mob_alive > 0) {
        return;
    }

    if (mob_wave_index_ < wave_cfg_.mob_waves_before_boss) {
        if (wave_intermission_rem_ > 0.0) {
            wave_intermission_rem_ -= dt;
            if (wave_intermission_rem_ <= 0.0) {
                wave_intermission_rem_ = 0.0;
                scatterObstacles();
                ++mob_wave_index_;
                spawnEnemiesForWave(mob_wave_index_);
            }
        } else {
            wave_intermission_rem_ = wave_cfg_.wave_intermission_sec;
        }
        return;
    }

    playfield_.clearObstaclesToFloor();

    const float wf = static_cast<float>(playfield_.width());
    const float hf = static_cast<float>(playfield_.height());
    const float cx0 = wf * 0.5f;
    const float cy0 = hf * 0.5f - kBossSpawnAboveCenterTiles;

    auto boss = std::make_unique<EnemyActor>();
    boss->x_ = cx0;
    boss->y_ = cy0;
    boss->resetForSpawn(EnemyArchetype::Boss, EnemySpriteId::BossCat);
    enemies_.push_back(std::move(boss));
    player_bullet_max_travel_world_ = wave_combat::kPlayerBossBulletTravelWorld;
    boss_released_ = true;
    boss_add_spawn_rem_ = wave_combat::kBossPhaseAddSpawnIntervalSec;
}

void World::tickBossPhaseAdds(double dt) {
    if (!boss_released_) {
        return;
    }
    if (!livingBossPresent(enemies_)) {
        return;
    }
    boss_add_spawn_rem_ -= dt;
    if (boss_add_spawn_rem_ > 0.0) {
        return;
    }
    trySpawnBossPhaseAdd();
    boss_add_spawn_rem_ = wave_combat::kBossPhaseAddSpawnIntervalSec;
}

void World::trySpawnBossPhaseAdd() {
    if (!livingBossPresent(enemies_)) {
        return;
    }
    if (livingBossMinionCount(enemies_) >= wave_combat::kBossPhaseAddMaxAlive) {
        return;
    }
    for (int tries = 0; tries < kEnemySpawnMaxTries; ++tries) {
        const int x = rng_.uniformInt(1, playfield_.width() - 2);
        const int y = rng_.uniformInt(1, playfield_.height() - 2);
        if (!playfield_.walkable(x, y)) {
            continue;
        }
        const float ex = static_cast<float>(x) + kPlayerSpawnInset;
        const float ey = static_cast<float>(y) + kPlayerSpawnInset;
        if (std::abs(ex - player_.x_) + std::abs(ey - player_.y_) < kBossPhaseAddSpawnManhattanMin) {
            continue;
        }
        if (!enemyFitsAt(ex, ey, nullptr)) {
            continue;
        }
        auto m = std::make_unique<EnemyActor>();
        m->x_ = ex;
        m->y_ = ey;
        const EnemySpriteId sp =
            rng_.uniformInt(0, 1) == 0 ? EnemySpriteId::Slime : EnemySpriteId::BugBit;
        m->resetForSpawn(EnemyArchetype::BossMinion, sp);
        enemies_.push_back(std::move(m));
        return;
    }
}

void World::simulateStep(double dt) {
    if (outcome_ != BattleOutcome::None) {
        return;
    }
    frame_intent_ = intent_;
    intent_ = {};

    player_.step(*this, dt);
    updatePickupsAndCollect(dt);
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
    maybeSpawnBossAfterWaveClear(dt);
    tickBossPhaseAdds(dt);
    evaluateBattleOutcome();
}

void World::spawnPlayerBullet(float x, float y, float vx, float vy, int damage, std::uint8_t player_bullet_visual) {
    const float r = player_bullet_max_travel_world_;
    const float max_sq = r > 0.f ? r * r : -1.f;
    bullets_.push_back(
        std::make_unique<PlayerBulletActor>(x, y, vx, vy, damage, max_sq, player_bullet_visual));
}

void World::cyclePlayerCharacter() {
    player_.character_id_ = (player_.character_id_ == PlayerCharacterId::Role1) ? PlayerCharacterId::Role2
                                                                                : PlayerCharacterId::Role1;
}

void World::setPlayerCharacter(PlayerCharacterId id) {
    player_.character_id_ = id;
}

void World::spawnEnemyBullet(float x, float y, float vx, float vy, int damage, EnemyBulletSprite sprite,
                             std::uint8_t boss_bullet_strip, float max_travel_sq) {
    bullets_.push_back(
        std::make_unique<EnemyBulletActor>(x, y, vx, vy, damage, sprite, boss_bullet_strip, -1.0, 0.f, max_travel_sq));
}

void World::spawnEnemyBulletSoftHoming(float x, float y, float vx, float vy, int damage, EnemyBulletSprite sprite,
                                       double straight_sec, float max_turn_rad_per_sec,
                                       std::uint8_t boss_bullet_strip) {
    bullets_.push_back(std::make_unique<EnemyBulletActor>(x, y, vx, vy, damage, sprite, boss_bullet_strip,
                                                           straight_sec, max_turn_rad_per_sec, -1.f));
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
    maybeSpawnPickupOnKill(enemy);
}

bool World::tryPlacePickup(PickupKind kind, float base_x, float base_y) {
    for (int attempt = 0; attempt < pickup::kPickupSpawnNudgeTries; ++attempt) {
        float ox = base_x;
        float oy = base_y;
        if (attempt > 0) {
            ox += static_cast<float>(rng_.uniformInt(-2, 2)) * 0.2f;
            oy += static_cast<float>(rng_.uniformInt(-2, 2)) * 0.2f;
        }
        if (terrainCircleWalkable(ox, oy, pickup::kPickupBodyRadiusWorld)) {
            pickups_.push_back(PickupDrop{kind, ox, oy});
            return true;
        }
    }
    return false;
}

void World::maybeSpawnPickupOnKill(const EnemyActor& enemy) {
    const bool boss = enemy.archetype() == EnemyArchetype::Boss;
    const int roll = rng_.uniformInt(0, 99);
    const int pr = boss ? pickup::kBossDropRedPct : pickup::kMobDropRedPct;
    const int pb = boss ? pickup::kBossDropBluePct : pickup::kMobDropBluePct;
    if (roll < pr) {
        tryPlacePickup(PickupKind::RedHeart, enemy.x(), enemy.y());
    } else if (roll < pr + pb) {
        tryPlacePickup(PickupKind::BlueHeart, enemy.x(), enemy.y());
    }
}

void World::updatePickupsAndCollect(double dt) {
    (void)dt;
    const float rs = pickup::kPickupRadiusWorld;
    const float rsq = rs * rs;
    for (std::size_t i = 0; i < pickups_.size();) {
        const PickupDrop& p = pickups_[i];
        const float dx = player_.x_ - p.x;
        const float dy = player_.y_ - p.y;
        if (lengthSq(dx, dy) > rsq) {
            ++i;
            continue;
        }
        if (p.kind == PickupKind::RedHeart) {
            if (player_.hp_ >= player_.max_hp_) {
                ++i;
                continue;
            }
            player_.healHp(pickup::kRedHeartHealHp);
        } else {
            if (player_.mp_ >= player_.max_mp_) {
                ++i;
                continue;
            }
            player_.restoreMp(pickup::kBlueHeartHealMp);
        }
        pickups_[i] = pickups_.back();
        pickups_.pop_back();
    }
}

void World::setPlayerDamageScoreBrackets(const PlayerDamageScoreBrackets& b) {
    player_damage_brackets_ = b;
    if (player_damage_brackets_.tier2_score <= player_damage_brackets_.tier1_score) {
        player_damage_brackets_ = PlayerDamageScoreBrackets{};
    }
    player_damage_brackets_.tier1_mult = std::max(1.f, player_damage_brackets_.tier1_mult);
    player_damage_brackets_.tier2_mult = std::max(1.f, player_damage_brackets_.tier2_mult);
}

void World::setWaveRuntimeConfig(const WaveRuntimeConfig& c) {
    wave_cfg_ = c;
}

float World::playerOutgoingDamageMultiplier() const {
    const int s = score_.totalScore();
    if (s > player_damage_brackets_.tier2_score) {
        return player_damage_brackets_.tier2_mult;
    }
    if (s > player_damage_brackets_.tier1_score) {
        return player_damage_brackets_.tier1_mult;
    }
    return 1.f;
}

void World::onBossDamagedByPlayer(int scaled_hp_lost) {
    if (scaled_hp_lost > 0) {
        score_.addBossChipScore(scaled_hp_lost);
    }
}

void World::devKillAllEnemies() {
    for (auto& e : enemies_) {
        if (!e || e->hp() <= 0) {
            continue;
        }
        e->applyDamage(e->maxHp() + e->maxHp() + 50, this);
    }
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
    if (!any && boss_released_) {
        outcome_ = BattleOutcome::Victory;
    }
}

int World::battleHudMobWave() const {
    if (!boss_released_ && wave_intermission_rem_ > 0.0 && mob_wave_index_ < wave_cfg_.mob_waves_before_boss) {
        const int next = mob_wave_index_ + 1;
        return std::min(next, wave_cfg_.mob_waves_before_boss);
    }
    return mob_wave_index_;
}

int World::mobWavesTotal() const {
    return wave_cfg_.mob_waves_before_boss;
}

int World::enemiesAliveCount() const {
    int n = 0;
    for (const auto& e : enemies_) {
        if (e && e->hp() > 0) {
            ++n;
        }
    }
    return n;
}

} // namespace domain
