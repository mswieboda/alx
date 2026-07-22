#pragma once

#include <vector>
#include <cstdint>

namespace alx {

// Enum representing the different types of grid elements in Aetherlux
enum class TileType : uint8_t {
    Empty = 0,
    Floor,
    Wall,
    Seep,       // Raw resource node (dark purple-black mana source)
    Pipe,       // Player-laid infrastructure pipe
    Refiner     // Conversion station (turns dark mana into cyan/gold light)
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
        }
    }

    // TODO: tmp, unused, old
    void update_mana_flow() {
        // 1. Reset power/mana state across all active conduits for a fresh calculation pass
        for (auto& tile : m_tiles) {
            if (tile.type == TileType::Pipe) {
                tile.mana_state = ManaState::None;
                tile.is_powered = false;
            } else if (tile.type == TileType::Refiner) {
                tile.is_powered = false; // Will light up if connected to a pipe carrying raw mana
            }
        }

        // 2. Simple Propagation: Scan for Seeps and push mana to adjacent pipes
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                const Tile& current = get_tile(x, y);

                // If we found a raw resource node, propagate to neighbors
                if (current.type == TileType::Seep) {
                    propagate_from(x, y, ManaState::Dark);
                }
            }
        }
    }

    void tick_simulation() {
        // We want to evaluate what each pipe should do based on its neighbors.
        // To prevent a chain reaction from fully propagating in a single tick,
        // we can either use a temporary buffer or evaluate a propagation front.
        // A clean grid-based approach for gradual flow is checking adjacent inputs:

        // Temporary storage or a two-pass check so flow moves 1 tile per tick
        std::vector<ManaState> next_mana_states(m_tiles.size(), ManaState::None);
        std::vector<bool> next_powered(m_tiles.size(), false);

        // Preserve Seeps and Refiners base states during the check
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                int idx = y * m_width + x;
                const Tile& current = m_tiles[idx];

                // Seeps always output Dark mana
                if (current.type == TileType::Seep) {
                    next_mana_states[idx] = ManaState::Dark;
                    next_powered[idx] = true;
                    continue;
                }

                // Refiners keep their type, but check if an adjacent pipe is powered
                if (current.type == TileType::Refiner) {
                    next_powered[idx] = has_powered_adjacent_pipe(x, y);
                    next_mana_states[idx] = next_powered[idx] ? ManaState::Dark : ManaState::None;
                    continue;
                }

                // For Pipes: Check if any orthogonal neighbor is a Seep or an active powered Pipe/Refiner
                if (current.type == TileType::Pipe) {
                    if (has_active_upstream_neighbor(x, y)) {
                        next_mana_states[idx] = ManaState::Dark;
                        next_powered[idx] = true;
                    } else {
                        next_mana_states[idx] = ManaState::None;
                        next_powered[idx] = false;
                    }
                }
            }
        }

        // Apply the next states back to the grid buffer
        for (size_t i = 0; i < m_tiles.size(); ++i) {
            m_tiles[i].mana_state = next_mana_states[i];
            m_tiles[i].is_powered = next_powered[i];
        }
    }

private:
    // TODO: unused, old
    // Recursive or iterative helper to spread mana through adjacent pipes
    void propagate_from(int x, int y, ManaState state) {
        // Check 4 orthogonal neighbors (Up, Down, Left, Right)
        int dx[] = { 0, 0, -1, 1 };
        int dy[] = { -1, 1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                Tile& neighbor = get_tile(nx, ny);

                // If it's a pipe and hasn't been energized yet on this tick
                if (neighbor.type == TileType::Pipe && neighbor.mana_state == ManaState::None) {
                    neighbor.mana_state = state;
                    neighbor.is_powered = true;
                    propagate_from(nx, ny, state); // Continue flow downstream
                }
                // If it reaches a refiner, power it up!
                else if (neighbor.type == TileType::Refiner && !neighbor.is_powered) {
                    neighbor.is_powered = true;
                    neighbor.mana_state = state;
                }
            }
        }
    }

    bool has_active_upstream_neighbor(int x, int y) const {
        int dx[] = { 0, 0, -1, 1 };
        int dy[] = { -1, 1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (is_in_bounds(nx, ny)) {
                const Tile& neighbor = get_tile(nx, ny);
                // If neighbor is a Seep, or a pipe/refiner that is currently powered, we receive flow!
                if (neighbor.type == TileType::Seep ||
                    ((neighbor.type == TileType::Pipe || neighbor.type == TileType::Refiner) && neighbor.is_powered)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool has_powered_adjacent_pipe(int x, int y) const {
        int dx[] = { 0, 0, -1, 1 };
        int dy[] = { -1, 1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (is_in_bounds(nx, ny)) {
                const Tile& neighbor = get_tile(nx, ny);
                if (neighbor.type == TileType::Pipe && neighbor.is_powered) {
                    return true;
                }
            }
        }
        return false;
    }
};

} // namespace alx
