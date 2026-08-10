#pragma once

#include <vector>
#include <cstdint>

#include "Game.h"
#include "core/Transform.h"
#include "GridPos.h"
#include "Fixture.h"
#include "Tiles.h"
#include "ParticleSystem.h"

namespace alx {

struct NetworkSimResults {
    int spires_converted = 0;
    int refiners_processed = 0;
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
    int find_empty_adjacent_pipe(int x, int y, const std::vector<Fixture>& next_fixtures, int& out_chosen_dir_idx) const;
    int find_active_input_pipe(int x, int y, ManaState target_state, int& out_chosen_dir_idx) const;
    int find_downstream_pipe_neighbor(int x, int y, ManaState state, const std::vector<int>& seep_dist, const std::vector<int>& light_dist, const std::vector<Fixture>& next_fixtures, int& out_chosen_dir_idx) const;

    struct DownstreamNeighbor { int idx; int dir_idx; };
    int find_all_downstream_neighbors(int x, int y, ManaState state, const std::vector<int>& seep_dist, const std::vector<int>& light_dist, const std::vector<Fixture>& next_fixtures, DownstreamNeighbor out_neighbors[4]) const;

    // --- Private Simulation Sub-Step Helpers ---
    void sim_consume(std::vector<Fixture>& next_fixtures);
    void sim_pipe_flow(const std::vector<int>& seep_dist, const std::vector<int>& light_dist, std::vector<Fixture>& next_fixtures);
    void sim_produce(NetworkSimResults& results, std::vector<Fixture>& next_fixtures);

public:
    Network(int width = 20, int height = 15, int tile_size = Game::TILE_SIZE);

    void resize(int width, int height);
    void clear();

    [[nodiscard]] bool in_bounds(GridPos pos) const noexcept;
    [[nodiscard]] bool in_bounds(int x, int y) const noexcept;

    [[nodiscard]] Fixture& fixture(GridPos pos) noexcept;
    [[nodiscard]] const Fixture& fixture(GridPos pos) const noexcept;
    [[nodiscard]] Fixture& fixture(int x, int y) noexcept;
    [[nodiscard]] const Fixture& fixture(int x, int y) const noexcept;

    // --- Placement & Removal Management ---
    bool can_place_fixture(GridPos pos, FixtureType type, const Tiles& tiles) const noexcept;
    bool place_fixture(GridPos pos, FixtureType type);
    bool remove_fixture(GridPos pos);
    bool damage_fixture(GridPos pos, int amount, float& out_twilight_increase);

    // --- Fixture Collision ---
    [[nodiscard]] bool is_solid(GridPos pos) const noexcept;
    [[nodiscard]] bool is_solid(int x, int y) const noexcept;

    // --- Fixture Type Groupings ---
    [[nodiscard]] bool is_building(FixtureType type) const noexcept;

    // --- Auto-Tiling Mask & Downstream Query ---
    [[nodiscard]] bool is_valid_port_connection(int from_x, int from_y, int to_x, int to_y) const noexcept;
    void update_neighbor_masks(GridPos pos);
    void downstream_dir(int x, int y, ManaState state, int& out_dx, int& out_dy) const;
    [[nodiscard]] bool is_tall_fixture(int tx, int ty) const noexcept {
        if (!in_bounds(tx, ty)) return false;
        FixtureType t = fixture(tx, ty).type;
        return t == FixtureType::Spire || t == FixtureType::Refiner;
    }

    // --- Clean Top-Level Simulation Master Tick ---
    NetworkSimResults sim_tick();

    [[nodiscard]] int width() const noexcept { return m_width; }
    [[nodiscard]] int height() const noexcept { return m_height; }
    [[nodiscard]] int tile_size() const noexcept { return m_tile_size; }
    [[nodiscard]] const std::vector<int32_t>& active_indices() const noexcept { return m_active_indices; }

    void draw(
        int min_tx, int max_tx, int min_ty, int max_ty,
        Transform p_xform, float progress,
        ParticleSystem* particle_system = nullptr, float last_dt = 0.016f, const float sim_tick_rate = 0.6f
    );
};

} // namespace alx
