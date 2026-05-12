#include "domain/skill.hpp"

#include "domain/combat_entities.hpp"
#include "domain/vec2.hpp"
#include "domain/world.hpp"

#include <cmath>

namespace domain {

namespace {

constexpr int kRingBulletCount = 28;

} // namespace

int RingBurstSkill::mpCost() const {
    return 10;
}

double RingBurstSkill::cooldownSeconds() const {
    return 0.0;
}

void RingBurstSkill::execute(SkillCastContext& ctx) const {
    if (ctx.caster != SkillCasterKind::Player) {
        return;
    }
    const float cx = ctx.player_foot_x;
    const float cy = ctx.player_foot_y - player_shot::kFeetToCenterWorld;
    for (int i = 0; i < kRingBulletCount; ++i) {
        const float a =
            (6.2831853071795864769f * static_cast<float>(i)) / static_cast<float>(kRingBulletCount);
        float bx = std::cos(a);
        float by = std::sin(a);
        normalizeOrDefault(bx, by);
        const float ox = cx + bx * player_shot::kMuzzleOffsetWorld;
        const float oy = cy + by * player_shot::kMuzzleOffsetWorld;
        ctx.world.spawnPlayerBullet(ox, oy, bx * player_shot::kBulletSpeed, by * player_shot::kBulletSpeed,
                                    ctx.bullet_damage);
    }
}

const ISkill& ringBurstSkill() {
    static const RingBurstSkill k{};
    return k;
}

} // namespace domain
