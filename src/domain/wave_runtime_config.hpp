#pragma once

namespace domain {

/** Mob-wave pacing loaded from `game_config.ini` (defaults match former `wave_combat_tuning` constants). */
struct WaveRuntimeConfig {
    int mob_waves_before_boss{3};
    double wave_intermission_sec{2.0};
    int mob_spawn_base_count{5};
    int mob_spawn_per_wave_extra{2};
    int mob_spawn_max_count{16};
    int scatter_obstacle_base{9};
    int scatter_obstacle_extra_roll{5};
};

} // namespace domain
