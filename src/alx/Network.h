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

    // --- Private Distance & Helper Methods ---
    std::vector<int> compute_distance_field(FixtureType sourceType) const;
    int find_empty_adjacent_pipe(int x, int y, const std::vector<Fixture>& next_fixtures) const;
    int find_active_input_pipe(int x, int y, ManaState target_state) const;
    int find_downstream_pipe_neighbor(int x, int y, ManaState state, const std::vector<int>& dark_dist, const std::vector<int>& light_dist, const std::vector<Fixture>& next_fixtures) const;

    // --- Private Simulation Sub-Step Helpers ---
    void sim_consume(std::vector<Fixture>& next_fixtures);
    void sim_pipe_flow(const std::vector<int>& dark_dist, const std::vector<int>& light_dist, std::vector<Fixture>& next_fixtures);
    void sim_produce(NetworkSimResults& results, std::vector<Fixture>& next_fixtures);

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

    // --- Auto-Tiling Mask & Downstream Query ---
    void update_neighbor_masks(GridPos pos);
    void get_downstream_dir(int x, int y, ManaState state, int& out_dx, int& out_dy) const;

    // --- Clean Top-Level Simulation Master Tick ---
    NetworkSimResults sim_tick();

    [[nodiscard]] int get_width() const noexcept { return m_width; }
    [[nodiscard]] int get_height() const noexcept { return m_height; }
    [[nodiscard]] int get_tile_size() const noexcept { return m_tile_size; }
    [[nodiscard]] const std::vector<int32_t>& get_active_indices() const noexcept { return m_active_indices; }
};

} // namespace alx
