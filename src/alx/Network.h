#pragma once

#include <vector>
#include <cstdint>

#include "GridPos.h"
#include "Fixture.h"
#include "Tiles.h"
#include "Game.h"

namespace alx {

struct NetworkSimResults {
    int spires_converted = 0;
    int stagnant_decay_events = 0;
};

class Network {
private:
    int m_width = 20;
    int m_height = 15;
    int m_tile_size = Game::TILE_SIZE;

    std::vector<Fixture> m_fixtures;
    std::vector<int32_t> m_active_indices;

    static constexpr uint8_t STAGNANT_DECAY_THRESHOLD = 20; // Ticks before stagnant mana turns into twilight

    // --- Private Simulation Sub-Step Helpers ---
    void sim_process_consumers(std::vector<Fixture>& next_fixtures, NetworkSimResults& results);
    void sim_pipe_flow(std::vector<Fixture>& next_fixtures);
    void sim_produce_sources(std::vector<Fixture>& next_fixtures);
    void sim_stagnant_decay(std::vector<Fixture>& next_fixtures, NetworkSimResults& results);

public:
    Network(int width = 20, int height = 15, int tile_size = Game::TILE_SIZE);

    void resize(int width, int height);
    void clear();

    [[nodiscard]] bool in_bounds(GridPos pos) const noexcept;
    [[nodiscard]] bool in_bounds(int x, int y) const noexcept;

    [[nodiscard]] Fixture& get_fixture(GridPos pos) noexcept;
    [[nodiscard]] const Fixture& get_fixture(GridPos pos) const noexcept;
    [[nodiscard]] Fixture& get_fixture(int x, int y) noexcept;
    [[nodiscard]] const Fixture& get_fixture(int x, int y) const noexcept;

    // --- Placement & Removal Management ---
    bool can_place_fixture(GridPos pos, FixtureType type, const Tiles& tiles) const noexcept;
    bool place_fixture(GridPos pos, FixtureType type);
    bool remove_fixture(GridPos pos);

    // --- Fixture Collision ---
    [[nodiscard]] bool is_solid(GridPos pos) const noexcept;
    [[nodiscard]] bool is_solid(int x, int y) const noexcept;

    // --- Auto-Tiling Mask & Gradient Updates ---
    void update_neighbor_masks(GridPos pos);
    void recalculate_flow_gradients();

    // --- Clean Top-Level Simulation Master Tick ---
    NetworkSimResults sim_tick();

    [[nodiscard]] int get_width() const noexcept { return m_width; }
    [[nodiscard]] int get_height() const noexcept { return m_height; }
    [[nodiscard]] int get_tile_size() const noexcept { return m_tile_size; }
    [[nodiscard]] const std::vector<int32_t>& get_active_indices() const noexcept { return m_active_indices; }
};

} // namespace alx
