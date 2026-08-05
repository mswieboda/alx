#pragma once
#include <vector>
#include <cstdint>
#include "Fixture.h"
#include "Network.h"

namespace alx {

class ParticleSystem;
struct Player;

namespace DrawFixtures {
    void begin_frame(float dt, float sim_tick_rate);

    void pipe(
        const Network& network, ParticleSystem& ps, const Fixture& fix,
        int gx, int gy, int world_x, int world_y, int tile_size,
        float progress, float dt, float sim_tick_rate
    );

    void building(
        const Network& network, ParticleSystem& ps, const Fixture& fix,
        int world_x, int world_y,
        float progress, float last_dt, float sim_tick_rate
    );

    void seep(const Fixture& fix, int world_x, int world_y, int tile_size);
} // namespace DrawFixtures
} // namespace alx
