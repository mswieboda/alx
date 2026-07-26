#include "Network.h"
#include <queue>
#include <algorithm>
#include "core/Log.h"

namespace alx {

Network::Network(int width, int height, int tile_size)
    : m_width(width), m_height(height), m_tile_size(tile_size)
{
    m_fixtures.resize(m_width * m_height, Fixture{});
}

void Network::resize(int width, int height) {
    m_width = width;
    m_height = height;
    m_fixtures.assign(m_width * m_height, Fixture{});
    m_active_indices.clear();
}

void Network::clear() {
    std::fill(m_fixtures.begin(), m_fixtures.end(), Fixture{});
    m_active_indices.clear();
}

bool Network::in_bounds(GridPos pos) const noexcept {
    return pos.x >= 0 && pos.x < m_width && pos.y >= 0 && pos.y < m_height;
}

bool Network::in_bounds(int x, int y) const noexcept {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

Fixture& Network::get_fixture(GridPos pos) noexcept {
    return m_fixtures[pos.to_index(m_width)];
}

const Fixture& Network::get_fixture(GridPos pos) const noexcept {
    return m_fixtures[pos.to_index(m_width)];
}

Fixture& Network::get_fixture(int x, int y) noexcept {
    return m_fixtures[y * m_width + x];
}

const Fixture& Network::get_fixture(int x, int y) const noexcept {
    return m_fixtures[y * m_width + x];
}

bool Network::can_place_fixture(GridPos pos, FixtureType type, const Tiles& tiles) const noexcept {
    if (!in_bounds(pos)) return false;
    if (!tiles.is_floor(pos)) return false;
    return get_fixture(pos).is_empty();
}

bool Network::place_fixture(GridPos pos, FixtureType type) {
    if (!in_bounds(pos)) return false;
    int32_t idx = pos.to_index(m_width);

    Fixture& fix = m_fixtures[idx];
    fix.type = type;
    fix.mana_state = ManaState::None;
    fix.process_timer = 0;
    fix.flags = 0;

    if (type == FixtureType::Seep) {
        fix.set_powered(true);
        fix.mana_state = ManaState::Dark;
    }

    if (std::find(m_active_indices.begin(), m_active_indices.end(), idx) == m_active_indices.end()) {
        m_active_indices.push_back(idx);
    }

    update_neighbor_masks(pos);
    recalculate_flow_gradients();
    return true;
}

bool Network::remove_fixture(GridPos pos) {
    if (!in_bounds(pos)) return false;
    int32_t idx = pos.to_index(m_width);

    auto it = std::find(m_active_indices.begin(), m_active_indices.end(), idx);
    if (it != m_active_indices.end()) {
        *it = m_active_indices.back();
        m_active_indices.pop_back();
    }

    m_fixtures[idx] = Fixture{};
    update_neighbor_masks(pos);
    recalculate_flow_gradients();
    return true;
}

bool Network::is_solid(GridPos pos) const noexcept {
    if (!in_bounds(pos)) return false;
    FixtureType t = get_fixture(pos).type;
    return t == FixtureType::Refiner || t == FixtureType::Spire || t == FixtureType::Seep;
}

bool Network::is_solid(int x, int y) const noexcept {
    return is_solid(GridPos{ x, y });
}

void Network::update_neighbor_masks(GridPos pos) {
    GridPos neighbors[4] = { pos.north(), pos.east(), pos.south(), pos.west() };
    uint8_t masks[4] = { DirectionMask::North, DirectionMask::East, DirectionMask::South, DirectionMask::West };

    for (int i = 0; i < 5; ++i) {
        GridPos target = (i == 4) ? pos : neighbors[i];
        if (!in_bounds(target)) continue;

        Fixture& fix = get_fixture(target);
        if (fix.is_empty()) continue;

        uint8_t in_mask = 0;
        GridPos t_neighbors[4] = { target.north(), target.east(), target.south(), target.west() };

        for (int k = 0; k < 4; ++k) {
            if (in_bounds(t_neighbors[k])) {
                const Fixture& n_fix = get_fixture(t_neighbors[k]);
                if (!n_fix.is_empty()) {
                    in_mask |= masks[k];
                }
            }
        }
        fix.flow_in_mask = in_mask;
    }
}

void Network::recalculate_flow_gradients() {
    std::vector<int32_t> dist(m_fixtures.size(), 9999);
    std::queue<int32_t> q;

    for (int32_t idx : m_active_indices) {
        FixtureType t = m_fixtures[idx].type;
        if (t == FixtureType::Refiner || t == FixtureType::Spire) {
            dist[idx] = 0;
            q.push(idx);
        }
    }

    while (!q.empty()) {
        int32_t curr = q.front();
        q.pop();

        GridPos curr_pos = GridPos::from_index(curr, m_width);
        int32_t curr_dist = dist[curr];

        GridPos n_pos[4] = { curr_pos.north(), curr_pos.east(), curr_pos.south(), curr_pos.west() };
        for (int i = 0; i < 4; ++i) {
            if (in_bounds(n_pos[i])) {
                int32_t n_idx = n_pos[i].to_index(m_width);
                if (m_fixtures[n_idx].type == FixtureType::Pipe) {
                    if (dist[n_idx] > curr_dist + 1) {
                        dist[n_idx] = curr_dist + 1;
                        q.push(n_idx);
                    }
                }
            }
        }
    }

    for (int32_t idx : m_active_indices) {
        Fixture& fix = m_fixtures[idx];
        if (fix.type != FixtureType::Pipe) continue;

        GridPos pos = GridPos::from_index(idx, m_width);
        GridPos n_pos[4] = { pos.north(), pos.east(), pos.south(), pos.west() };
        int8_t dir_dx[4] = { 0, 1, 0, -1 };
        int8_t dir_dy[4] = { -1, 0, 1, 0 };

        int32_t best_d = dist[idx];
        int8_t best_dx = 0;
        int8_t best_dy = 0;

        for (int i = 0; i < 4; ++i) {
            if (in_bounds(n_pos[i])) {
                int32_t n_idx = n_pos[i].to_index(m_width);
                if (dist[n_idx] < best_d) {
                    best_d = dist[n_idx];
                    best_dx = dir_dx[i];
                    best_dy = dir_dy[i];
                }
            }
        }

        fix.move_dx = best_dx;
        fix.move_dy = best_dy;
    }
}

void Network::sim_process_consumers(std::vector<Fixture>& next_fixtures, NetworkSimResults& results) {
    for (int32_t idx : m_active_indices) {
        Fixture& curr = m_fixtures[idx];
        if (curr.type == FixtureType::Refiner) {
            GridPos pos = GridPos::from_index(idx, m_width);
            GridPos n_pos[4] = { pos.north(), pos.east(), pos.south(), pos.west() };

            for (int i = 0; i < 4; ++i) {
                if (in_bounds(n_pos[i])) {
                    int32_t n_idx = n_pos[i].to_index(m_width);
                    if (next_fixtures[n_idx].type == FixtureType::Pipe && next_fixtures[n_idx].mana_state == ManaState::Dark) {
                        next_fixtures[n_idx].mana_state = ManaState::None;
                        next_fixtures[idx].set_powered(true);
                        next_fixtures[idx].mana_state = ManaState::Light;
                        break;
                    }
                }
            }

            if (next_fixtures[idx].is_powered() && next_fixtures[idx].mana_state == ManaState::Light) {
                for (int i = 0; i < 4; ++i) {
                    if (in_bounds(n_pos[i])) {
                        int32_t n_idx = n_pos[i].to_index(m_width);
                        if (next_fixtures[n_idx].type == FixtureType::Pipe && next_fixtures[n_idx].mana_state == ManaState::None) {
                            next_fixtures[n_idx].mana_state = ManaState::Light;
                            next_fixtures[n_idx].set_powered(true);
                            next_fixtures[idx].set_powered(false);
                            next_fixtures[idx].mana_state = ManaState::None;
                            break;
                        }
                    }
                }
            }
        }
        else if (curr.type == FixtureType::Spire) {
            GridPos pos = GridPos::from_index(idx, m_width);
            GridPos n_pos[4] = { pos.north(), pos.east(), pos.south(), pos.west() };

            for (int i = 0; i < 4; ++i) {
                if (in_bounds(n_pos[i])) {
                    int32_t n_idx = n_pos[i].to_index(m_width);
                    if (next_fixtures[n_idx].type == FixtureType::Pipe && next_fixtures[n_idx].mana_state == ManaState::Light) {
                        next_fixtures[n_idx].mana_state = ManaState::None;
                        results.spires_converted++;
                        break;
                    }
                }
            }
        }
    }
}

void Network::sim_pipe_flow(std::vector<Fixture>& next_fixtures) {
    for (int32_t idx : m_active_indices) {
        const Fixture& curr = m_fixtures[idx];
        if (curr.type == FixtureType::Pipe && curr.mana_state != ManaState::None) {
            if (curr.move_dx != 0 || curr.move_dy != 0) {
                GridPos pos = GridPos::from_index(idx, m_width);
                GridPos dest_pos{ static_cast<int16_t>(pos.x + curr.move_dx), static_cast<int16_t>(pos.y + curr.move_dy) };

                if (in_bounds(dest_pos)) {
                    int32_t dest_idx = dest_pos.to_index(m_width);
                    if (next_fixtures[dest_idx].type == FixtureType::Pipe && next_fixtures[dest_idx].mana_state == ManaState::None) {
                        next_fixtures[dest_idx].mana_state = curr.mana_state;
                        next_fixtures[dest_idx].set_powered(true);
                        next_fixtures[dest_idx].process_timer = 0;

                        next_fixtures[idx].mana_state = ManaState::None;
                        next_fixtures[idx].set_powered(false);
                        next_fixtures[idx].process_timer = 0;
                    }
                }
            }
        }
    }
}

void Network::sim_produce_sources(std::vector<Fixture>& next_fixtures) {
    for (int32_t idx : m_active_indices) {
        if (m_fixtures[idx].type == FixtureType::Seep) {
            GridPos pos = GridPos::from_index(idx, m_width);
            GridPos n_pos[4] = { pos.north(), pos.east(), pos.south(), pos.west() };

            for (int i = 0; i < 4; ++i) {
                if (in_bounds(n_pos[i])) {
                    int32_t n_idx = n_pos[i].to_index(m_width);
                    if (next_fixtures[n_idx].type == FixtureType::Pipe && next_fixtures[n_idx].mana_state == ManaState::None) {
                        next_fixtures[n_idx].mana_state = ManaState::Dark;
                        next_fixtures[n_idx].set_powered(true);
                        break;
                    }
                }
            }
        }
    }
}

void Network::sim_stagnant_decay(std::vector<Fixture>& next_fixtures, NetworkSimResults& results) {
    for (int32_t idx : m_active_indices) {
        Fixture& fix = next_fixtures[idx];
        if (fix.type == FixtureType::Pipe && fix.mana_state != ManaState::None) {
            if (fix.move_dx == 0 && fix.move_dy == 0) {
                fix.process_timer++;
                if (fix.process_timer >= STAGNANT_DECAY_THRESHOLD) {
                    fix.mana_state = ManaState::None;
                    fix.set_powered(false);
                    fix.process_timer = 0;
                    results.stagnant_decay_events++;
                }
            }
        }
    }
}

NetworkSimResults Network::sim_tick() {
    NetworkSimResults results{};
    std::vector<Fixture> next_fixtures = m_fixtures;

    sim_process_consumers(next_fixtures, results);
    sim_pipe_flow(next_fixtures);
    sim_produce_sources(next_fixtures);
    sim_stagnant_decay(next_fixtures, results);

    m_fixtures = std::move(next_fixtures);
    return results;
}

} // namespace alx
