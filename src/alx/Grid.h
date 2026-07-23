#pragma once

#include <vector>
#include <cstdint>
#include <queue>

namespace alx {

// Enum representing the different types of grid elements in Aetherlux
enum class TileType : uint8_t {
    Empty = 0,
    Floor,
    Wall,
    Seep,       // Produces dark mana, and releases twilight into room
    Pipe,       // Player-laid infrastructure pipe
    Refiner,    // Converts dark mana into light mana
    LightSpire  // Converts light mana into light to fight room twilight
};

// State for pipes or nodes to handle power/mana routing logic later
enum class ManaState : uint8_t {
    None = 0,
    Dark,       // Volatile dark twilight mana
    Light  // Stable light energy
};

struct Tile {
    TileType type = TileType::Empty;
    ManaState mana_state = ManaState::None;
    bool is_powered = false;
    uint8_t process_timer = 0;
};

class Grid {
private:
    int m_width;
    int m_height;
    int m_tile_size; // e.g., 16 pixels per tile for GBA aesthetic
    std::vector<Tile> m_tiles;

public:
    Grid(int width, int height, int tileSize)
        : m_width(width), m_height(height), m_tile_size(tileSize) {
        m_tiles.resize(m_width * m_height);
    }

    static bool has_mana_glow(const Tile& tile) {
        return tile.type == TileType::Pipe
            || tile.type == TileType::Seep
            || tile.type == TileType::Refiner;
    }

    static bool has_power_glow(const Tile& tile) {
        return tile.type == TileType::Refiner;
    }

    bool is_in_bounds(int x, int y) const {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    }

    // Coordinate indexing helper
    Tile& get_tile(int x, int y) {
        return m_tiles[y * m_width + x];
    }

    const Tile& get_tile(int x, int y) const {
        return m_tiles[y * m_width + x];
    }

    bool is_walkable(int x, int y) const {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height) return false;
        TileType t = get_tile(x, y).type;
        // Player can walk on floors, around pipes, and near seeps; walls block movement
        return t != TileType::Wall && t != TileType::Empty;
    }

    int get_width() const { return m_width; }
    int get_height() const { return m_height; }
    int get_tile_size() const { return m_tile_size; }

    // Expanded toggle or specific placement method
    void place_tile(int tx, int ty, TileType type) {
        if (tx < 0 || tx >= m_width || ty < 0 || ty >= m_height) return;

        Tile& tile = get_tile(tx, ty);
        if (tile.type == TileType::Floor || tile.type == TileType::Empty) {
            tile.type = type;
        } else if (tile.type == type) {
            // Pick it back up if clicking the same thing
            tile.type = TileType::Floor;
            tile.mana_state = ManaState::None;
            tile.is_powered = false;
            tile.process_timer = 0;
        }
    }

    void tick_simulation() {
        std::vector<ManaState> next_mana_states(m_tiles.size(), ManaState::None);
        std::vector<bool> next_powered(m_tiles.size(), false);
        std::vector<uint8_t> next_process_timers(m_tiles.size(), 0);

        std::vector<int> dark_dist = compute_distance_field(TileType::Seep);
        std::vector<int> light_dist = compute_distance_field(TileType::Refiner);

        // --- PASS 1: Evaluate fixed sources (Seeps, Refiners, and Spires) ---
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                int idx = y * m_width + x;
                const Tile& current = m_tiles[idx];

                if (current.type == TileType::Seep) {
                    next_mana_states[idx] = ManaState::Dark;
                    next_powered[idx] = true;
                }
                else if (current.type == TileType::Refiner) {
                    if (has_active_neighbor_with_state(x, y, ManaState::Dark)) {
                        next_powered[idx] = true;
                        next_mana_states[idx] = ManaState::Dark;

                        uint8_t progress = current.process_timer + 1;
                        if (progress >= 3) {
                            push_light_mana_forward(x, y, next_mana_states, next_powered);
                            progress = 0;
                        }
                        next_process_timers[idx] = progress;
                    } else {
                        next_powered[idx] = false;
                        next_mana_states[idx] = ManaState::None;
                        next_process_timers[idx] = 0;
                    }
                }
                else if (current.type == TileType::LightSpire) {
                    if (has_active_neighbor_with_state(x, y, ManaState::Light)) {
                        next_powered[idx] = true;
                        next_mana_states[idx] = ManaState::Light;
                    }
                }
            }
        }

        // --- PASS 2: Sequential Pipe Propagation ---
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                int idx = y * m_width + x;
                const Tile& current = m_tiles[idx];

                if (current.type == TileType::Pipe) {
                    ManaState flow = get_valid_flow_source(x, y, dark_dist, light_dist);
                    if (flow != ManaState::None) {
                        next_mana_states[idx] = flow;
                        next_powered[idx] = true;
                    }
                }
            }
        }

        // Commit buffer states back to the grid
        for (size_t i = 0; i < m_tiles.size(); ++i) {
            m_tiles[i].mana_state = next_mana_states[i];
            m_tiles[i].is_powered = next_powered[i];
            m_tiles[i].process_timer = next_process_timers[i];
        }
    }

private:
    std::vector<int> compute_distance_field(TileType sourceType) const {
        std::vector<int> dist(m_tiles.size(), 9999);
        std::queue<int> q;

        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                int idx = y * m_width + x;
                if (m_tiles[idx].type == sourceType) {
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

                if (is_in_bounds(nx, ny)) {
                    int n_idx = ny * m_width + nx;
                    if (m_tiles[n_idx].type == TileType::Pipe) {
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

    bool has_active_neighbor_with_state(int x, int y, ManaState target_state) const {
        int dx[] = { 0, 0, -1, 1 };
        int dy[] = { -1, 1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (is_in_bounds(nx, ny)) {
                const Tile& neighbor = get_tile(nx, ny);
                if (neighbor.is_powered && neighbor.mana_state == target_state) {
                    return true;
                }
            }
        }
        return false;
    }

    ManaState get_valid_flow_source(int x, int y, const std::vector<int>& dark_dist, const std::vector<int>& light_dist) const {
        int idx = y * m_width + x;
        int my_dark_d = dark_dist[idx];
        int my_light_d = light_dist[idx];

        int dx[] = { 0, 0, -1, 1 };
        int dy[] = { -1, 1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (is_in_bounds(nx, ny)) {
                const Tile& neighbor = get_tile(nx, ny);
                int n_idx = ny * m_width + nx;

                if (neighbor.is_powered && neighbor.mana_state != ManaState::None) {
                    if (neighbor.mana_state == ManaState::Dark && dark_dist[n_idx] < my_dark_d) {
                        return ManaState::Dark;
                    }
                    if (neighbor.mana_state == ManaState::Light && light_dist[n_idx] < my_light_d) {
                        return ManaState::Light;
                    }
                }
            }
        }
        return ManaState::None;
    }

    void push_light_mana_forward(int refiner_x, int refiner_y, std::vector<ManaState>& next_mana_states, std::vector<bool>& next_powered) {
        int dx[] = { 0, 0, -1, 1 };
        int dy[] = { -1, 1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            int nx = refiner_x + dx[i];
            int ny = refiner_y + dy[i];

            if (is_in_bounds(nx, ny)) {
                const Tile& neighbor = get_tile(nx, ny);
                if (neighbor.type == TileType::Pipe) {
                    int n_idx = ny * m_width + nx;
                    if (next_mana_states[n_idx] == ManaState::None) {
                        next_mana_states[n_idx] = ManaState::Light;
                        next_powered[n_idx] = true;
                        break;
                    }
                }
            }
        }
    }
};

} // namespace alx
