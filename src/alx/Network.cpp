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
    if (!in_bounds(pos)) return false;
    if (!tiles.is_floor(pos)) return false;
    return fixture(pos).is_empty();
}

bool Network::place_fixture(GridPos pos, FixtureType type) {
    if (!in_bounds(pos)) return false;
    int32_t idx = pos.to_index(m_width);

    Fixture& fix = m_fixtures[idx];
    fix.type = type;
    fix.mana_state = ManaState::None;
    fix.process_timer = 0;
    fix.mana_ttl = 0;
    fix.is_powered = false;

    int max_hp = 0;
    if (type == FixtureType::Pipe) max_hp = FixtureHPConstants::PIPE_MAX_HP;
    else if (type == FixtureType::Refiner) max_hp = FixtureHPConstants::REFINER_MAX_HP;
    else if (type == FixtureType::Spire) max_hp = FixtureHPConstants::SPIRE_MAX_HP;

    fix.max_hp = max_hp;
    fix.hp = max_hp;

    if (type == FixtureType::Seep) {
        fix.is_powered = true;
        fix.mana_state = ManaState::Dark;
    }

    if (std::find(m_active_indices.begin(), m_active_indices.end(), idx) == m_active_indices.end()) {
        m_active_indices.push_back(idx);
    }

    update_neighbor_masks(pos);
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
    return true;
}

bool Network::damage_fixture(GridPos pos, int amount, float& out_twilight_increase) {
    out_twilight_increase = 0.0f;
    if (!in_bounds(pos)) return false;
    Fixture& fix = fixture(pos);
    if (fix.is_empty()) return false;

    fix.hp -= amount;
    if (fix.hp <= 0) {
        FixtureType t = fix.type;
        if (t == FixtureType::Pipe) {
            out_twilight_increase = 0.05f;
        } else if (t == FixtureType::Refiner || t == FixtureType::Spire) {
            out_twilight_increase = 0.15f;
        }
        remove_fixture(pos);
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
                const Fixture& n_fix = fixture(t_neighbors[k]);
                if (!n_fix.is_empty()) {
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

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int idx = y * m_width + x;
            if (m_fixtures[idx].type == sourceType) {
                dist[idx] = 0;
                q.push(idx);
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
    std::vector<int> light_dist = compute_distance_field(FixtureType::Refiner);

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
                    best_d = dark_dist[n_idx];
                    out_dx = dx[i];
                    out_dy = dy[i];
                }
            }
        }
    }
}

int Network::find_empty_adjacent_pipe(int x, int y, const std::vector<Fixture>& next_fixtures, int& out_chosen_dir_idx) const {
    out_chosen_dir_idx = -1;
    if (!in_bounds(x, y)) return -1;
    int idx = y * m_width + x;
    uint8_t start_dir = m_fixtures[idx].last_dir_idx;

    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    for (int step = 1; step <= 4; ++step) {
        int i = (start_dir + step) % 4;
        int nx = x + dx[i];
        int ny = y + dy[i];

        if (in_bounds(nx, ny)) {
            int n_idx = ny * m_width + nx;
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
    int idx = y * m_width + x;
    uint8_t start_dir = m_fixtures[idx].last_dir_idx;

    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    for (int step = 1; step <= 4; ++step) {
        int i = (start_dir + step) % 4;
        int nx = x + dx[i];
        int ny = y + dy[i];

        if (in_bounds(nx, ny)) {
            int n_idx = ny * m_width + nx;
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
    const std::vector<int>& dark_dist,
    const std::vector<int>& light_dist,
    const std::vector<Fixture>& next_fixtures,
    int& out_chosen_dir_idx
) const {
    out_chosen_dir_idx = -1;
    int idx = y * m_width + x;
    uint8_t start_dir = m_fixtures[idx].last_dir_idx;

    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    const auto& dist = (state == ManaState::Dark) ? dark_dist : light_dist;
    int my_d = dist[idx];

    if (my_d < 9000) {
        for (int step = 1; step <= 4; ++step) {
            int i = (start_dir + step) % 4;
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (in_bounds(nx, ny)) {
                int n_idx = ny * m_width + nx;
                const Fixture& n = m_fixtures[n_idx];
                if (n.type == FixtureType::Pipe && dist[n_idx] > my_d && dist[n_idx] < 9000) {
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

void Network::sim_consume(std::vector<Fixture>& next_fixtures) {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int idx = y * m_width + x;
            const Fixture& current = m_fixtures[idx];

            if (current.type == FixtureType::Refiner) {
                if (current.process_timer == 0) {
                    int chosen_dir = -1;
                    int in_pipe_idx = find_active_input_pipe(x, y, ManaState::Dark, chosen_dir);
                    if (in_pipe_idx != -1) {
                        next_fixtures[idx].last_dir_idx = static_cast<uint8_t>(chosen_dir);

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
            else if (current.type == FixtureType::Spire) {
                if (current.process_timer == 0) {
                    int chosen_dir = -1;
                    int in_pipe_idx = find_active_input_pipe(x, y, ManaState::Light, chosen_dir);
                    if (in_pipe_idx != -1) {
                        next_fixtures[idx].last_dir_idx = static_cast<uint8_t>(chosen_dir);

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
    const std::vector<int>& dark_dist,
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
                dark_pipes.push_back({ x, y, idx, dark_dist[idx] });
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

        if (dark_dist[idx] < 9000) {
            // Connected to active Seep -> Advance flow 1 tile downstream per tick
            next.is_draining = false;

            int chosen_dir = -1;
            int downstream_idx = find_downstream_pipe_neighbor(x, y, ManaState::Dark, dark_dist, light_dist, next_fixtures, chosen_dir);
            if (downstream_idx != -1) {
                next.last_dir_idx = static_cast<uint8_t>(chosen_dir);
                int downstream_x = downstream_idx % m_width;
                int downstream_y = downstream_idx / m_width;
                next.out_dx = static_cast<int8_t>(downstream_x - x);
                next.out_dy = static_cast<int8_t>(downstream_y - y);

                // Advance Dark Mana to downstream pipe on this tick!
                if (next_fixtures[downstream_idx].mana_state == ManaState::None) {
                    next_fixtures[downstream_idx].mana_state = ManaState::Dark;
                    next_fixtures[downstream_idx].is_powered = true;
                    next_fixtures[downstream_idx].is_draining = false;
                    next_fixtures[downstream_idx].move_dx = static_cast<int8_t>(downstream_x - x);
                    next_fixtures[downstream_idx].move_dy = static_cast<int8_t>(downstream_y - y);
                }
            } else {
                int out_dx = 0, out_dy = 0;
                downstream_dir(x, y, ManaState::Dark, out_dx, out_dy);
                next.out_dx = static_cast<int8_t>(out_dx);
                next.out_dy = static_cast<int8_t>(out_dy);
            }
        } else {
            // Disconnected from Seep -> Enter Draining State!
            next.is_draining = true;

            int in_x = x - next.move_dx;
            int in_y = y - next.move_dy;
            if (!in_bounds(in_x, in_y) || m_fixtures[in_y * m_width + in_x].mana_state == ManaState::None || m_fixtures[in_y * m_width + in_x].is_empty()) {
                next.mana_state = ManaState::None;
                next.is_powered = false;
                next.is_draining = false;
                next.move_dx = 0;
                next.move_dy = 0;
                next.out_dx = 0;
                next.out_dy = 0;
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
        return a.dist > b.dist;
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
            next_fixtures[idx].out_dx = 0;
            next_fixtures[idx].out_dy = 0;
            continue;
        }

        int chosen_dir = -1;
        int downstream_idx = find_downstream_pipe_neighbor(x, y, ManaState::Light, dark_dist, light_dist, next_fixtures, chosen_dir);

        if (downstream_idx != -1) {
            next_fixtures[idx].last_dir_idx = static_cast<uint8_t>(chosen_dir);

            int downstream_x = downstream_idx % m_width;
            int downstream_y = downstream_idx / m_width;

            next_fixtures[idx].out_dx = static_cast<int8_t>(downstream_x - x);
            next_fixtures[idx].out_dy = static_cast<int8_t>(downstream_y - y);

            next_fixtures[downstream_idx].mana_state = ManaState::Light;
            next_fixtures[downstream_idx].is_powered = true;
            next_fixtures[downstream_idx].mana_ttl = curr_ttl - 1;
            next_fixtures[downstream_idx].move_dx = static_cast<int8_t>(downstream_x - x);
            next_fixtures[downstream_idx].move_dy = static_cast<int8_t>(downstream_y - y);

            next_fixtures[idx].mana_state = ManaState::None;
            next_fixtures[idx].is_powered = false;
            next_fixtures[idx].move_dx = 0;
            next_fixtures[idx].move_dy = 0;
        } else {
            next_fixtures[idx].mana_state = ManaState::Light;
            next_fixtures[idx].is_powered = true;
            next_fixtures[idx].mana_ttl = curr_ttl - 1;
            next_fixtures[idx].move_dx = 0;
            next_fixtures[idx].move_dy = 0;
            next_fixtures[idx].out_dx = 0;
            next_fixtures[idx].out_dy = 0;
        }
    }
}

void Network::sim_produce(NetworkSimResults& results, std::vector<Fixture>& next_fixtures) {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int idx = y * m_width + x;
            const Fixture& current = m_fixtures[idx];

            if (current.type == FixtureType::Seep) {
                next_fixtures[idx].mana_state = ManaState::Dark;
                next_fixtures[idx].is_powered = true;

                int chosen_dir = -1;
                int open_pipe_idx = find_empty_adjacent_pipe(x, y, next_fixtures, chosen_dir);
                if (open_pipe_idx != -1) {
                    next_fixtures[idx].last_dir_idx = static_cast<uint8_t>(chosen_dir);

                    int open_x = open_pipe_idx % m_width;
                    int open_y = open_pipe_idx / m_width;

                    next_fixtures[open_pipe_idx].mana_state = ManaState::Dark;
                    next_fixtures[open_pipe_idx].is_powered = true;
                    next_fixtures[open_pipe_idx].move_dx = static_cast<int8_t>(open_x - x);
                    next_fixtures[open_pipe_idx].move_dy = static_cast<int8_t>(open_y - y);
                }
            }
            else if (current.type == FixtureType::Refiner) {
                if (next_fixtures[idx].process_timer > 0) {
                    next_fixtures[idx].is_powered = true;
                    next_fixtures[idx].mana_state = ManaState::Dark;

                    uint8_t progress = next_fixtures[idx].process_timer + 1;
                    if (progress >= Game::REFINER_TICKS_REQUIRED) {
                        int chosen_dir = -1;
                        int out_pipe_idx = find_empty_adjacent_pipe(x, y, next_fixtures, chosen_dir);
                        if (out_pipe_idx != -1) {
                            next_fixtures[idx].last_dir_idx = static_cast<uint8_t>(chosen_dir);

                            int out_x = out_pipe_idx % m_width;
                            int out_y = out_pipe_idx / m_width;

                            next_fixtures[out_pipe_idx].mana_state = ManaState::Light;
                            next_fixtures[out_pipe_idx].is_powered = true;
                            next_fixtures[out_pipe_idx].mana_ttl = Game::LIGHT_MANA_TIME_TO_LIFE_TICKS;
                            next_fixtures[out_pipe_idx].move_dx = static_cast<int8_t>(out_x - x);
                            next_fixtures[out_pipe_idx].move_dy = static_cast<int8_t>(out_y - y);
                            progress = 0;
                            next_fixtures[idx].is_powered = false;
                            next_fixtures[idx].mana_state = ManaState::None;
                        } else {
                            progress = Game::REFINER_TICKS_REQUIRED;
                        }
                    }
                    next_fixtures[idx].process_timer = progress;
                } else if (next_fixtures[idx].mana_state == ManaState::None) {
                    next_fixtures[idx].is_powered = false;
                    next_fixtures[idx].process_timer = 0;
                }
            }
            else if (current.type == FixtureType::Spire) {
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

    std::vector<int> dark_dist = compute_distance_field(FixtureType::Seep);
    std::vector<int> light_dist = compute_distance_field(FixtureType::Refiner);

    sim_consume(next_fixtures);
    sim_pipe_flow(dark_dist, light_dist, next_fixtures);
    sim_produce(results, next_fixtures);

    m_fixtures = std::move(next_fixtures);
    return results;
}

} // namespace alx
