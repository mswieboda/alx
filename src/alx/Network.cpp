#include "Network.h"
#include <queue>
#include <algorithm>
#include "Debug.h"
#include "Layer.h"
#include "core/Draw.h"
#include "core/Log.h"
#include "core/Transform.h"
#include "DrawFixtures.h"
#include "Random.h"
#include "alx/ParticleEmitters.h"

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

Fixture& Network::fixture(GridPos pos) noexcept {
    return m_fixtures[pos.to_index(m_width)];
}

const Fixture& Network::fixture(GridPos pos) const noexcept {
    return m_fixtures[pos.to_index(m_width)];
}

Fixture& Network::fixture(int x, int y) noexcept {
    return m_fixtures[y * m_width + x];
}

const Fixture& Network::fixture(int x, int y) const noexcept {
    return m_fixtures[y * m_width + x];
}

bool Network::can_place_fixture(GridPos pos, FixtureType type, const Tiles& tiles) const noexcept {
    MultiTileFootprint fp = get_fixture_footprint(type);
    for (int dy = 0; dy < fp.height; ++dy) {
        for (int dx = 0; dx < fp.width; ++dx) {
            GridPos target{ static_cast<int16_t>(pos.x + dx), static_cast<int16_t>(pos.y + dy) };
            if (!in_bounds(target)) return false;
            if (!tiles.is_floor(target)) return false;
            if (!fixture(target).is_empty()) return false;
        }
    }
    return true;
}

bool Network::place_fixture(GridPos pos, FixtureType type) {
    if (!in_bounds(pos)) return false;
    MultiTileFootprint fp = get_fixture_footprint(type);

    int max_hp = 0;
    if (type == FixtureType::Pipe) max_hp = FixtureHPConstants::PIPE_MAX_HP;
    else if (type == FixtureType::Refiner) max_hp = FixtureHPConstants::REFINER_MAX_HP;
    else if (type == FixtureType::Spire) max_hp = FixtureHPConstants::SPIRE_MAX_HP;

    int32_t root_idx = pos.to_index(m_width);

    for (int dy = 0; dy < fp.height; ++dy) {
        for (int dx = 0; dx < fp.width; ++dx) {
            GridPos target{ static_cast<int16_t>(pos.x + dx), static_cast<int16_t>(pos.y + dy) };
            if (!in_bounds(target)) continue;
            int32_t idx = target.to_index(m_width);

            Fixture& fix = m_fixtures[idx];
            fix.type = type;
            fix.mana_state = ManaState::None;
            fix.process_timer = 0;
            fix.mana_ttl = 0;
            fix.is_powered = false;
            fix.root_offset_x = static_cast<int8_t>(dx);
            fix.root_offset_y = static_cast<int8_t>(dy);

            if (dx == 0 && dy == 0) {
                fix.max_hp = max_hp;
                fix.hp = max_hp;
                if (type == FixtureType::Seep) {
                    fix.is_powered = true;
                    fix.mana_state = ManaState::Dark;
                }
                if (std::find(m_active_indices.begin(), m_active_indices.end(), root_idx) == m_active_indices.end()) {
                    m_active_indices.push_back(root_idx);
                }
            }
        }
    }

    for (int dy = -1; dy <= fp.height; ++dy) {
        for (int dx = -1; dx <= fp.width; ++dx) {
            GridPos p{ static_cast<int16_t>(pos.x + dx), static_cast<int16_t>(pos.y + dy) };
            if (in_bounds(p)) update_neighbor_masks(p);
        }
    }
    return true;
}

bool Network::remove_fixture(GridPos pos) {
    if (!in_bounds(pos)) return false;
    const Fixture& target_fix = fixture(pos);
    if (target_fix.is_empty()) return false;

    GridPos root_pos{ static_cast<int16_t>(pos.x - target_fix.root_offset_x), static_cast<int16_t>(pos.y - target_fix.root_offset_y) };
    if (!in_bounds(root_pos)) return false;

    FixtureType root_type = fixture(root_pos).type;
    MultiTileFootprint fp = get_fixture_footprint(root_type);
    int32_t root_idx = root_pos.to_index(m_width);

    auto it = std::find(m_active_indices.begin(), m_active_indices.end(), root_idx);
    if (it != m_active_indices.end()) {
        *it = m_active_indices.back();
        m_active_indices.pop_back();
    }

    for (int dy = 0; dy < fp.height; ++dy) {
        for (int dx = 0; dx < fp.width; ++dx) {
            GridPos p{ static_cast<int16_t>(root_pos.x + dx), static_cast<int16_t>(root_pos.y + dy) };
            if (in_bounds(p)) {
                m_fixtures[p.to_index(m_width)] = Fixture{};
            }
        }
    }

    for (int dy = -1; dy <= fp.height; ++dy) {
        for (int dx = -1; dx <= fp.width; ++dx) {
            GridPos p{ static_cast<int16_t>(root_pos.x + dx), static_cast<int16_t>(root_pos.y + dy) };
            if (in_bounds(p)) update_neighbor_masks(p);
        }
    }
    return true;
}

bool Network::damage_fixture(GridPos pos, int amount, float& out_twilight_increase) {
    out_twilight_increase = 0.0f;
    if (!in_bounds(pos)) return false;
    const Fixture& target_fix = fixture(pos);
    if (target_fix.is_empty()) return false;

    GridPos root_pos{ static_cast<int16_t>(pos.x - target_fix.root_offset_x), static_cast<int16_t>(pos.y - target_fix.root_offset_y) };
    if (!in_bounds(root_pos)) return false;
    Fixture& root_fix = fixture(root_pos);

    root_fix.hp -= amount;
    if (root_fix.hp <= 0) {
        FixtureType t = root_fix.type;
        if (t == FixtureType::Pipe) {
            out_twilight_increase = 0.05f;
        } else if (t == FixtureType::Refiner || t == FixtureType::Spire) {
            out_twilight_increase = 0.15f;
        }
        remove_fixture(root_pos);
        return true;
    }
    return false;
}

bool Network::is_solid(GridPos pos) const noexcept {
    if (!in_bounds(pos)) return false;
    FixtureType t = fixture(pos).type;
    return t == FixtureType::Refiner || t == FixtureType::Spire;
}

bool Network::is_solid(int x, int y) const noexcept {
    return is_solid(GridPos{ x, y });
}

bool Network::is_building(FixtureType type) const noexcept {
    return type == FixtureType::Refiner
        || type == FixtureType::Spire;
}

bool Network::is_valid_port_connection(int from_x, int from_y, int to_x, int to_y) const noexcept {
    if (!in_bounds(from_x, from_y) || !in_bounds(to_x, to_y)) return false;
    int delta_x = to_x - from_x;
    int delta_y = to_y - from_y;
    if (std::abs(delta_x) + std::abs(delta_y) != 1) return false;

    const Fixture& from_fix = fixture(from_x, from_y);
    const Fixture& to_fix = fixture(to_x, to_y);

    if (from_fix.is_empty() || to_fix.is_empty()) return false;

    // Case 1: Pipe to Pipe
    if (from_fix.type == FixtureType::Pipe && to_fix.type == FixtureType::Pipe) {
        return true;
    }

    // Case 2: Pipe at (from_x, from_y) connecting to Node building at (to_x, to_y)
    if (from_fix.type == FixtureType::Pipe && to_fix.type != FixtureType::Pipe) {
        return is_fixture_port(
            to_fix.type, to_fix.root_offset_x, to_fix.root_offset_y,
            static_cast<int8_t>(-delta_x), static_cast<int8_t>(-delta_y)
        );
    }

    // Case 3: Node building at (from_x, from_y) connecting to Pipe at (to_x, to_y)
    if (from_fix.type != FixtureType::Pipe && to_fix.type == FixtureType::Pipe) {
        return is_fixture_port(
            from_fix.type, from_fix.root_offset_x, from_fix.root_offset_y,
            static_cast<int8_t>(delta_x), static_cast<int8_t>(delta_y)
        );
    }

    return false;
}

void Network::update_neighbor_masks(GridPos pos) {
    GridPos neighbors[4] = { pos.north(), pos.east(), pos.south(), pos.west() };
    uint8_t masks[4] = { DirectionMask::North, DirectionMask::East, DirectionMask::South, DirectionMask::West };

    for (int i = 0; i < 5; ++i) {
        GridPos target = (i == 4) ? pos : neighbors[i];
        if (!in_bounds(target)) continue;

        Fixture& fix = fixture(target);
        if (fix.is_empty()) continue;

        uint8_t in_mask = 0;
        GridPos t_neighbors[4] = { target.north(), target.east(), target.south(), target.west() };

        for (int k = 0; k < 4; ++k) {
            if (in_bounds(t_neighbors[k])) {
                if (is_valid_port_connection(target.x, target.y, t_neighbors[k].x, t_neighbors[k].y)) {
                    in_mask |= masks[k];
                }
            }
        }
        fix.flow_in_mask = in_mask;
    }
}

std::vector<int> Network::compute_distance_field(FixtureType sourceType) const {
    std::vector<int> dist(m_fixtures.size(), 9999);
    std::queue<int> q;

    PortLocation ports[4];

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int idx = y * m_width + x;
            const Fixture& fix = m_fixtures[idx];
            if (fix.type == sourceType && fix.is_root()) {
                int port_count = get_fixture_ports(sourceType, ports);
                for (int p = 0; p < port_count; ++p) {
                    int p_x = x + ports[p].off_x;
                    int p_y = y + ports[p].off_y;
                    int p_idx = p_y * m_width + p_x;
                    dist[p_idx] = 0;

                    int target_x = p_x + ports[p].face_dx;
                    int target_y = p_y + ports[p].face_dy;
                    if (in_bounds(target_x, target_y)) {
                        int t_idx = target_y * m_width + target_x;
                        if (m_fixtures[t_idx].type == FixtureType::Pipe) {
                            if (dist[t_idx] > 1) {
                                dist[t_idx] = 1;
                                q.push(t_idx);
                            }
                        }
                    }
                }
            }
        }
    }

    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    while (!q.empty()) {
        int curr = q.front();
        q.pop();

        int cx = curr % m_width;
        int cy = curr / m_width;
        int curr_dist = dist[curr];

        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            if (in_bounds(nx, ny)) {
                int n_idx = ny * m_width + nx;
                if (m_fixtures[n_idx].type == FixtureType::Pipe) {
                    if (dist[n_idx] > curr_dist + 1) {
                        dist[n_idx] = curr_dist + 1;
                        q.push(n_idx);
                    }
                } else if (is_building(m_fixtures[n_idx].type)) {
                    if (is_valid_port_connection(cx, cy, nx, ny)) {
                        if (dist[n_idx] > curr_dist + 1) {
                            dist[n_idx] = curr_dist + 1;
                        }
                    }
                }
            }
        }
    }

    return dist;
}

void Network::downstream_dir(int x, int y, ManaState state, int& out_dx, int& out_dy) const {
    out_dx = 0;
    out_dy = 0;

    if (!in_bounds(x, y)) return;
    const Fixture& current = fixture(x, y);
    if (current.type != FixtureType::Pipe || current.mana_state == ManaState::None) return;

    int idx = y * m_width + x;
    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    std::vector<int> dark_dist = compute_distance_field(FixtureType::Seep);

    if (state == ManaState::Dark) {
        int my_d = dark_dist[idx];
        int best_d = my_d;
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (in_bounds(nx, ny)) {
                int n_idx = ny * m_width + nx;
                const Fixture& n = m_fixtures[n_idx];
                if ((n.type == FixtureType::Pipe || n.type == FixtureType::Refiner) && dark_dist[n_idx] > best_d && dark_dist[n_idx] < 9000) {
                    if (is_valid_port_connection(x, y, nx, ny)) {
                        best_d = dark_dist[n_idx];
                        out_dx = dx[i];
                        out_dy = dy[i];
                    }
                }
            }
        }
    }
}

int Network::find_empty_adjacent_pipe(int x, int y, const std::vector<Fixture>& next_fixtures, int& out_chosen_dir_idx) const {
    out_chosen_dir_idx = -1;
    if (!in_bounds(x, y)) return -1;
    const Fixture& root_fix = fixture(x, y);
    if (!root_fix.is_root()) return -1;

    PortLocation ports[4];
    int port_count = get_fixture_ports(root_fix.type, ports);
    if (port_count <= 0) return -1;
    uint8_t start_dir = root_fix.last_out_dir_idx;

    for (int step = 1; step <= port_count; ++step) {
        int i = (start_dir + step) % port_count;
        const auto& p = ports[i];
        int px = x + p.off_x + p.face_dx;
        int py = y + p.off_y + p.face_dy;

        if (in_bounds(px, py)) {
            int n_idx = py * m_width + px;
            const Fixture& neighbor = m_fixtures[n_idx];
            if (neighbor.type == FixtureType::Pipe && neighbor.mana_state == ManaState::None && next_fixtures[n_idx].mana_state == ManaState::None) {
                out_chosen_dir_idx = i;
                return n_idx;
            }
        }
    }
    return -1;
}

int Network::find_active_input_pipe(int x, int y, ManaState target_state, int& out_chosen_dir_idx) const {
    out_chosen_dir_idx = -1;
    if (!in_bounds(x, y)) return -1;
    const Fixture& root_fix = fixture(x, y);
    if (!root_fix.is_root()) return -1;

    PortLocation ports[4];
    int port_count = get_fixture_ports(root_fix.type, ports);
    if (port_count <= 0) return -1;
    uint8_t start_dir = root_fix.last_in_dir_idx;

    for (int step = 1; step <= port_count; ++step) {
        int i = (start_dir + step) % port_count;
        const auto& p = ports[i];
        int px = x + p.off_x + p.face_dx;
        int py = y + p.off_y + p.face_dy;

        if (in_bounds(px, py)) {
            int n_idx = py * m_width + px;
            const Fixture& neighbor = m_fixtures[n_idx];
            if (neighbor.type == FixtureType::Pipe && neighbor.is_powered && neighbor.mana_state == target_state) {
                out_chosen_dir_idx = i;
                return n_idx;
            }
        }
    }
    return -1;
}

int Network::find_downstream_pipe_neighbor(
    int x, int y, ManaState state,
    const std::vector<int>& seep_dist,
    const std::vector<int>& light_dist,
    const std::vector<Fixture>& next_fixtures,
    int& out_chosen_dir_idx
) const {
    out_chosen_dir_idx = -1;
    int idx = y * m_width + x;
    const Fixture& current = m_fixtures[idx];
    uint8_t start_dir = current.last_out_dir_idx;


    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    const auto& dist = (state == ManaState::Dark) ? seep_dist : light_dist;
    int my_d = dist[idx];
    FixtureType target_building = (state == ManaState::Dark) ? FixtureType::Refiner : FixtureType::Spire;

    // NRR: No-Reverse Rule - forbid candidate directions that are the exact 180° opposite of incoming velocity
    int back_dx = -current.move_dx;
    int back_dy = -current.move_dy;

    // Pass 1: SDF Sink Preference (if valid path towards sink exists)
    if (state == ManaState::Light && my_d < 9000) {
        for (int step = 1; step <= 4; ++step) {
            int i = (start_dir + step) % 4;
            if (current.move_dx != 0 || current.move_dy != 0) {
                if (dx[i] == back_dx && dy[i] == back_dy) continue; // Skip 180° reverse
            }
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (in_bounds(nx, ny)) {
                int n_idx = ny * m_width + nx;
                const Fixture& n = m_fixtures[n_idx];
                if ((n.type == FixtureType::Pipe || n.type == target_building) && dist[n_idx] < my_d) {
                    if (is_valid_port_connection(x, y, nx, ny)) {
                        if (next_fixtures[n_idx].mana_state == ManaState::None) {
                            out_chosen_dir_idx = i;
                            return n_idx;
                        }
                    }
                }
            }
        }
    }

    // Pass 2: Round-robin into ANY connected empty pipe (dead ends / un-sunk pipes)
    for (int step = 1; step <= 4; ++step) {
        int i = (start_dir + step) % 4;
        if (current.move_dx != 0 || current.move_dy != 0) {
            if (dx[i] == back_dx && dy[i] == back_dy) continue; // Skip 180° reverse
        }
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (in_bounds(nx, ny)) {
            int n_idx = ny * m_width + nx;
            const Fixture& n = m_fixtures[n_idx];
            if (n.type == FixtureType::Pipe || n.type == target_building) {
                if (is_valid_port_connection(x, y, nx, ny)) {
                    if (next_fixtures[n_idx].mana_state == ManaState::None) {
                        out_chosen_dir_idx = i;
                        return n_idx;
                    }
                }
            }
        }
    }

    return -1;
}

int Network::find_all_downstream_neighbors(
    int x, int y, ManaState state,
    const std::vector<int>& seep_dist,
    const std::vector<int>& light_dist,
    const std::vector<Fixture>& next_fixtures,
    DownstreamNeighbor out_neighbors[4]
) const {
    int idx = y * m_width + x;
    const Fixture& current = m_fixtures[idx];

    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    int back_dx = -current.move_dx;
    int back_dy = -current.move_dy;

    int count = 0;
    FixtureType target_building = (state == ManaState::Dark) ? FixtureType::Refiner : FixtureType::Spire;

    for (int i = 0; i < 4; ++i) {
        if (current.move_dx != 0 || current.move_dy != 0) {
            if (dx[i] == back_dx && dy[i] == back_dy) continue; // Skip 180° reverse
        }
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (in_bounds(nx, ny)) {
            int n_idx = ny * m_width + nx;
            const Fixture& n = m_fixtures[n_idx];
            if (n.type == FixtureType::Pipe || n.type == target_building) {
                if (is_valid_port_connection(x, y, nx, ny)) {
                    if (next_fixtures[n_idx].mana_state == ManaState::None) {
                        out_neighbors[count++] = { n_idx, i };
                    }
                }
            }
        }
    }
    return count;
}



void Network::sim_consume(std::vector<Fixture>& next_fixtures) {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int idx = y * m_width + x;
            const Fixture& current = m_fixtures[idx];

            if (current.type == FixtureType::Refiner && current.is_root()) {
                if (current.process_timer == 0 && current.mana_state == ManaState::None) {
                    int chosen_dir = -1;
                    int in_pipe_idx = find_active_input_pipe(x, y, ManaState::Dark, chosen_dir);
                    if (in_pipe_idx != -1) {
                        next_fixtures[idx].last_in_dir_idx = static_cast<uint8_t>(chosen_dir);

                        // If input pipe was draining, consumption advances its draining state
                        if (m_fixtures[in_pipe_idx].is_draining) {
                            next_fixtures[in_pipe_idx].mana_state = ManaState::None;
                            next_fixtures[in_pipe_idx].is_powered = false;
                        }

                        next_fixtures[idx].is_powered = true;
                        next_fixtures[idx].mana_state = ManaState::Dark;
                        next_fixtures[idx].process_timer = 1;
                    }
                }
            }
            else if (current.type == FixtureType::Spire && current.is_root()) {
                if (current.process_timer == 0) {
                    int chosen_dir = -1;
                    int in_pipe_idx = find_active_input_pipe(x, y, ManaState::Light, chosen_dir);
                    if (in_pipe_idx != -1) {
                        next_fixtures[idx].last_in_dir_idx = static_cast<uint8_t>(chosen_dir);

                        next_fixtures[in_pipe_idx].mana_state = ManaState::None;
                        next_fixtures[in_pipe_idx].is_powered = false;
                        next_fixtures[in_pipe_idx].mana_ttl = 0;
                        next_fixtures[in_pipe_idx].move_dx = 0;
                        next_fixtures[in_pipe_idx].move_dy = 0;

                        next_fixtures[idx].is_powered = true;
                        next_fixtures[idx].mana_state = ManaState::Light;
                        next_fixtures[idx].process_timer = 1;
                    }
                }
            }

        }
    }
}

void Network::sim_pipe_flow(
    const std::vector<int>& seep_dist,
    const std::vector<int>& light_dist,
    std::vector<Fixture>& next_fixtures
) {
    // 1. Process Iterative Dark Mana Pipeline Flow & Receding Draining
    struct DarkPipeIndex {
        int x, y, idx, dist;
    };
    std::vector<DarkPipeIndex> dark_pipes;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int idx = y * m_width + x;
            const Fixture& current = m_fixtures[idx];
            if (current.type == FixtureType::Pipe && current.mana_state == ManaState::Dark) {
                dark_pipes.push_back({ x, y, idx, seep_dist[idx] });
            }
        }
    }

    // Sort by distance from Seep ascending (closest to Seep first)
    std::sort(dark_pipes.begin(), dark_pipes.end(), [](const DarkPipeIndex& a, const DarkPipeIndex& b) {
        return a.dist < b.dist;
    });

    for (const auto& pipe_node : dark_pipes) {
        int idx = pipe_node.idx;
        int x = pipe_node.x;
        int y = pipe_node.y;
        Fixture& next = next_fixtures[idx];

        bool is_connected = (seep_dist[idx] < 9000);
        if (!is_connected) {
            next.is_draining = true;
        } else {
            next.is_draining = false;
        }

        // Advance Dark Mana to ALL downstream empty neighbors simultaneously
        DownstreamNeighbor neighbors[4];
        int neighbor_count = find_all_downstream_neighbors(x, y, ManaState::Dark, seep_dist, light_dist, next_fixtures, neighbors);

        if (neighbor_count > 0) {
            int dx[] = { 0, 0, -1, 1 };
            int dy[] = { -1, 1, 0, 0 };
            uint8_t out_mask = 0;

            for (int n = 0; n < neighbor_count; ++n) {
                int downstream_idx = neighbors[n].idx;
                int dir_idx = neighbors[n].dir_idx;
                int downstream_x = downstream_idx % m_width;
                int downstream_y = downstream_idx / m_width;

                out_mask |= DirectionMask::from_delta(dx[dir_idx], dy[dir_idx]);

                if (next_fixtures[downstream_idx].mana_state == ManaState::None) {
                    next_fixtures[downstream_idx].mana_state = ManaState::Dark;
                    next_fixtures[downstream_idx].is_powered = true;
                    next_fixtures[downstream_idx].is_draining = !is_connected;
                    next_fixtures[downstream_idx].move_dx = static_cast<int8_t>(downstream_x - x);
                    next_fixtures[downstream_idx].move_dy = static_cast<int8_t>(downstream_y - y);
                }
            }
            next.flow_out_mask = out_mask;
        } else {
            // Downstream fallback for draining pipes (distance >= 9000)
            int flow_dx = 0, flow_dy = 0;
            DirectionMask::to_delta(next.flow_out_mask, flow_dx, flow_dy);
            if (flow_dx == 0 && flow_dy == 0) {
                flow_dx = next.move_dx;
                flow_dy = next.move_dy;
            }

            if (flow_dx != 0 || flow_dy != 0) {
                int nx = x + flow_dx;
                int ny = y + flow_dy;
                if (in_bounds(nx, ny)) {
                    int n_idx = ny * m_width + nx;
                    if (next_fixtures[n_idx].type == FixtureType::Pipe && next_fixtures[n_idx].mana_state == ManaState::None) {
                        next.flow_out_mask = DirectionMask::from_delta(flow_dx, flow_dy);
                        next_fixtures[n_idx].mana_state = ManaState::Dark;
                        next_fixtures[n_idx].is_powered = true;
                        next_fixtures[n_idx].is_draining = true;
                        next_fixtures[n_idx].move_dx = static_cast<int8_t>(flow_dx);
                        next_fixtures[n_idx].move_dy = static_cast<int8_t>(flow_dy);
                    }
                }
            } else {
                int out_dx = 0, out_dy = 0;
                downstream_dir(x, y, ManaState::Dark, out_dx, out_dy);
                next.flow_out_mask = DirectionMask::from_delta(out_dx, out_dy);
            }
        }

        // Process upstream empty check for draining tail recession
        if (next.is_draining) {
            int in_x = x - next.move_dx;
            int in_y = y - next.move_dy;
            if (!in_bounds(in_x, in_y) || m_fixtures[in_y * m_width + in_x].mana_state == ManaState::None || m_fixtures[in_y * m_width + in_x].is_empty()) {
                next.mana_state = ManaState::None;
                next.is_powered = false;
                next.is_draining = false;
                next.move_dx = 0;
                next.move_dy = 0;
                next.flow_out_mask = 0;
            }
        }
    }

    // 2. Process Discrete Light Mana Orb Flow (Conduits to Spires)
    struct PipeIndex {
        int x, y, idx, dist;
    };
    std::vector<PipeIndex> light_pipes;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int idx = y * m_width + x;
            const Fixture& current = m_fixtures[idx];
            if (current.type == FixtureType::Pipe && current.is_powered && current.mana_state == ManaState::Light) {
                light_pipes.push_back({ x, y, idx, light_dist[idx] });
            }
        }
    }

    std::sort(light_pipes.begin(), light_pipes.end(), [](const PipeIndex& a, const PipeIndex& b) {
        return a.dist < b.dist;
    });

    for (const auto& pipe_node : light_pipes) {
        int idx = pipe_node.idx;
        int x = pipe_node.x;
        int y = pipe_node.y;

        if (next_fixtures[idx].mana_state != ManaState::Light) continue;

        uint8_t curr_ttl = next_fixtures[idx].mana_ttl;
        if (curr_ttl <= 1) {
            next_fixtures[idx].mana_state = ManaState::None;
            next_fixtures[idx].is_powered = false;
            next_fixtures[idx].mana_ttl = 0;
            next_fixtures[idx].move_dx = 0;
            next_fixtures[idx].move_dy = 0;
            next_fixtures[idx].flow_out_mask = 0;
            continue;
        }

        int chosen_dir = -1;
        int downstream_idx = find_downstream_pipe_neighbor(x, y, ManaState::Light, seep_dist, light_dist, next_fixtures, chosen_dir);

        if (downstream_idx != -1) {
            next_fixtures[idx].last_out_dir_idx = static_cast<uint8_t>(chosen_dir);

            int downstream_x = downstream_idx % m_width;
            int downstream_y = downstream_idx / m_width;

            next_fixtures[idx].flow_out_mask = DirectionMask::from_delta(downstream_x - x, downstream_y - y);

            next_fixtures[downstream_idx].mana_state = ManaState::Light;
            next_fixtures[downstream_idx].is_powered = true;
            next_fixtures[downstream_idx].mana_ttl = curr_ttl - 1;
            next_fixtures[downstream_idx].move_dx = static_cast<int8_t>(downstream_x - x);
            next_fixtures[downstream_idx].move_dy = static_cast<int8_t>(downstream_y - y);
            next_fixtures[downstream_idx].is_stepping = true;

            next_fixtures[idx].mana_state = ManaState::None;
            next_fixtures[idx].is_powered = false;
            next_fixtures[idx].move_dx = 0;
            next_fixtures[idx].move_dy = 0;
            next_fixtures[idx].is_stepping = false;
        } else {
            next_fixtures[idx].mana_state = ManaState::Light;
            next_fixtures[idx].is_powered = true;
            next_fixtures[idx].mana_ttl = curr_ttl - 1;
            next_fixtures[idx].move_dx = m_fixtures[idx].move_dx;
            next_fixtures[idx].move_dy = m_fixtures[idx].move_dy;
            next_fixtures[idx].is_stepping = false;
            next_fixtures[idx].flow_out_mask = 0;
        }

    }
}

void Network::sim_produce(NetworkSimResults& results, std::vector<Fixture>& next_fixtures) {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int idx = y * m_width + x;
            const Fixture& current = m_fixtures[idx];

            if (current.type == FixtureType::Seep && current.is_root()) {
                next_fixtures[idx].mana_state = ManaState::Dark;
                next_fixtures[idx].is_powered = true;

                PortLocation ports[4];
                int port_count = get_fixture_ports(FixtureType::Seep, ports);
                uint8_t out_mask = 0;
                uint8_t start_dir = current.last_out_dir_idx;

                for (int step = 1; step <= port_count; ++step) {
                    int i = (start_dir + step) % port_count;
                    const auto& p = ports[i];
                    int px = x + p.off_x + p.face_dx;
                    int py = y + p.off_y + p.face_dy;
                    if (in_bounds(px, py)) {
                        int n_idx = py * m_width + px;
                        if (m_fixtures[n_idx].type == FixtureType::Pipe) {
                            if (m_fixtures[n_idx].mana_state == ManaState::None
                                && next_fixtures[n_idx].mana_state == ManaState::None) {
                                next_fixtures[n_idx].mana_state = ManaState::Dark;
                                next_fixtures[n_idx].is_powered = true;
                                next_fixtures[n_idx].move_dx = p.face_dx;
                                next_fixtures[n_idx].move_dy = p.face_dy;
                                next_fixtures[idx].last_out_dir_idx = static_cast<uint8_t>(i);
                            }
                            if (next_fixtures[n_idx].mana_state == ManaState::Dark) {
                                out_mask |= DirectionMask::from_delta(p.face_dx, p.face_dy);
                            }
                        }
                    }
                }
                next_fixtures[idx].flow_out_mask = out_mask;
            }
            else if (current.type == FixtureType::Refiner && current.is_root()) {
                if (next_fixtures[idx].mana_state == ManaState::Dark) {
                    next_fixtures[idx].is_powered = true;
                    if (current.mana_state == ManaState::Dark) {
                        uint8_t progress = current.process_timer + 1;
                        if (progress >= Game::REFINER_PROCESSING_TICKS_REQUIRED) {
                            int chosen_dir = -1;
                            int out_pipe_idx = find_empty_adjacent_pipe(x, y, next_fixtures, chosen_dir);
                            if (out_pipe_idx != -1) {
                                next_fixtures[idx].last_out_dir_idx = static_cast<uint8_t>(chosen_dir);

                                next_fixtures[out_pipe_idx].mana_state = ManaState::Light;
                                next_fixtures[out_pipe_idx].is_powered = true;
                                next_fixtures[out_pipe_idx].mana_ttl = Game::LIGHT_MANA_TIME_TO_LIFE_TICKS;

                                PortLocation ports[4];
                                get_fixture_ports(FixtureType::Refiner, ports);
                                const auto& chosen_port = ports[chosen_dir];
                                next_fixtures[out_pipe_idx].move_dx = chosen_port.face_dx;
                                next_fixtures[out_pipe_idx].move_dy = chosen_port.face_dy;
                                next_fixtures[out_pipe_idx].is_stepping = true;

                                progress = Game::REFINER_CONSUMING_WAIT_TICKS;
                                next_fixtures[idx].is_powered = false;
                                next_fixtures[idx].mana_state = ManaState::None;
                            } else {
                                progress = Game::REFINER_PROCESSING_TICKS_REQUIRED;
                            }
                        }
                        next_fixtures[idx].process_timer = progress;
                    }
                } else if (current.mana_state == ManaState::None && current.process_timer > 0) {
                    next_fixtures[idx].is_powered = false;
                    next_fixtures[idx].mana_state = ManaState::None;
                    next_fixtures[idx].process_timer = current.process_timer - 1;
                } else if (next_fixtures[idx].mana_state == ManaState::None) {
                    next_fixtures[idx].is_powered = false;
                    next_fixtures[idx].process_timer = 0;
                }
            }
            else if (current.type == FixtureType::Spire && current.is_root()) {
                if (next_fixtures[idx].process_timer > 0) {
                    next_fixtures[idx].is_powered = true;
                    next_fixtures[idx].mana_state = ManaState::Light;

                    uint8_t progress = next_fixtures[idx].process_timer + 1;
                    if (progress >= Game::LIGHT_SPIRE_TICKS_REQUIRED) {
                        results.spires_converted++;

                        progress = 0;
                        next_fixtures[idx].is_powered = false;
                        next_fixtures[idx].mana_state = ManaState::None;
                    }
                    next_fixtures[idx].process_timer = progress;
                } else if (next_fixtures[idx].mana_state == ManaState::None) {
                    next_fixtures[idx].is_powered = false;
                    next_fixtures[idx].process_timer = 0;
                }
            }
        }
    }
}

NetworkSimResults Network::sim_tick() {
    NetworkSimResults results{};
    std::vector<Fixture> next_fixtures = m_fixtures;

    std::vector<int> seep_dist = compute_distance_field(FixtureType::Seep);
    std::vector<int> light_dist = compute_distance_field(FixtureType::Spire);

    sim_consume(next_fixtures);
    sim_pipe_flow(seep_dist, light_dist, next_fixtures);
    sim_produce(results, next_fixtures);

    m_fixtures = std::move(next_fixtures);
    return results;
}

void Network::draw(
    int min_tx, int max_tx, int min_ty, int max_ty,
    Transform p_xform, float progress,
    ParticleSystem& ps, float last_dt, const float sim_tick_rate
) {
    DrawFixtures::begin_frame(last_dt, sim_tick_rate);

    for (int gy = min_ty; gy <= max_ty; ++gy) {
        for (int gx = min_tx; gx <= max_tx; ++gx) {
            const Fixture& fix = fixture(gx, gy);
            if (fix.is_empty() || !fix.is_root()) continue;

            FixtureType type = fix.type;
            int world_x = gx * m_tile_size;
            int world_y = gy * m_tile_size;

            if (type == FixtureType::Pipe) {
                DrawFixtures::pipe(*this, ps, fix, gx, gy, world_x, world_y, m_tile_size, progress, last_dt, sim_tick_rate);
            } else if (is_building(type)) {
                DrawFixtures::building(
                    *this, ps, fix,
                    world_x, world_y,
                    progress, last_dt, sim_tick_rate
                );
            } else if (type == FixtureType::Seep) {
                DrawFixtures::seep(fix, world_x, world_y, m_tile_size);
                if (Random::chance(0.1f)) {
                    ParticleEmitters::spawn_dark_mana_spill(ps, world_x + 8.0f, world_y + 8.0f, Layer::WorldObjFX);
                }
            }

            if constexpr (Debug::DRAW_FIXTURE_COLLISION_AREAS) {
                if (is_solid(gx, gy)) {
                    Collision::AABB c_aabb = fixture_ground_aabb(gx, gy, static_cast<float>(m_tile_size), type);
                    Draw::rect(c_aabb.x, c_aabb.y, c_aabb.w, c_aabb.h, 0xFFFF00FF, false, 1, Layer::HUD_Text);
                }
            }
        }
    }
}

} // namespace alx
