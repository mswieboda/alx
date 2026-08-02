#pragma once
#include <vector>
#include <cstdint>
#include "Fixture.h"
#include "Network.h"

namespace alx {

class ParticleSystem;
struct Player;

namespace DrawFixtures {

    /// Renders base tile geometry for all network fixtures (pipe hubs, refiners, spires, seeps).
    void draw_backgrounds(const Network& network, int min_tx, int max_tx, int min_ty, int max_ty, const Player* player = nullptr);

    /// Renders non-particle static/pulsing fixture mana components (Light Mana Orbs, Refiner/Spire cores).
    void draw_mana(const Network& network, int min_tx, int max_tx, int min_ty, int max_ty, float progress, const Player* player = nullptr);

    /// Triggers particle emissions for active network fixtures (Pipes, Refiners, Spires).
    void emit_particles(ParticleSystem& ps, const Network& network, int min_tx, int max_tx, int min_ty, int max_ty, float dt, float sim_tick_rate);

} // namespace DrawFixtures
} // namespace alx
