#pragma once

#include <vector>
#include <cstdint>

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

    void tick_simulation() {
        std::vector<ManaState> next_mana_states(m_tiles.size(), ManaState::None);
        std::vector<bool> next_powered(m_tiles.size(), false);

        // Preserve fixed sources
        // Seeps produce Dark mana, Refiners process it, Spires consume Light mana)
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                int idx = y * m_width + x;
                const Tile& current = m_tiles[idx];

                // Seeps always output Dark mana
                if (current.type == TileType::Seep) {
                    next_mana_states[idx] = ManaState::Dark;
                    next_powered[idx] = true;
                    continue;
                } else if (current.type == TileType::Refiner) {
                    // A refiner powers on and processes if it receives Dark mana from an adjacent pipe/seep
                    // TODO: do only Pipes in future, not anything with Dark mana (like Seep)
                    bool has_dark_input = has_active_neighbor_with_state(x, y, ManaState::Dark);

                    if (has_dark_input) {
                        next_powered[idx] = true;
                        next_mana_states[idx] = ManaState::Dark;

                        // --- REFINER OUTPUT LOGIC ---
                        // Try to push Light mana into an adjacent empty/pipe tile
                        push_light_mana_to_neighbor(x, y);
                    } else {
                        next_powered[idx] = false;
                        next_mana_states[idx] = ManaState::None;
                    }
                } else if (current.type == TileType::LightSpire) {
                    // Spire powers on if it receives Light mana
                    bool has_light_input = has_active_neighbor_with_state(x, y, ManaState::Light);
                    next_powered[idx] = has_light_input;
                    next_mana_states[idx] = has_light_input ? ManaState::Light : ManaState::None;
                }
            }
        }

        // Second Pass: Mana Pipe Propagation (respecting fluid type separation)
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                int idx = y * m_width + x;
                const Tile& current = m_tiles[idx];

                if (current.type == TileType::Pipe) {
                    // Check what kind of fluid-yielding neighbor is nearby
                    ManaState incoming_fluid = get_compatible_upstream_fluid(x, y);
                    if (incoming_fluid != ManaState::None) {
                        next_mana_states[idx] = incoming_fluid;
                        next_powered[idx] = true;
                    } else {
                        next_mana_states[idx] = ManaState::None;
                        next_powered[idx] = false;
                    }
                }
            }
        }

        // Commit changes back to grid
        for (size_t i = 0; i < m_tiles.size(); ++i) {
            m_tiles[i].mana_state = next_mana_states[i];
            m_tiles[i].is_powered = next_powered[i];
        }
    }

private:
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

    ManaState get_compatible_upstream_fluid(int x, int y) const {
        int dx[] = { 0, 0, -1, 1 };
        int dy[] = { -1, 1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (is_in_bounds(nx, ny)) {
                const Tile& neighbor = get_tile(nx, ny);
                if (neighbor.is_powered && neighbor.mana_state != ManaState::None) {
                    // A pipe adopts the fluid state of its powered neighbor
                    return neighbor.mana_state;
                }
            }
        }

        return ManaState::None;
    }

    void push_light_mana_to_neighbor(int refiner_x, int refiner_y) {
        int dx[] = { 0, 0, -1, 1 };
        int dy[] = { -1, 1, 0, 0 };

        // Find an adjacent pipe or empty-ish tile to inject purified Light mana into
        for (int i = 0; i < 4; ++i) {
            int nx = refiner_x + dx[i];
            int ny = refiner_y + dy[i];

            if (is_in_bounds(nx, ny)) {
                Tile& neighbor = get_tile(nx, ny);

                // If it's a pipe carrying nothing or already carrying light, inject light
                if (neighbor.type == TileType::Pipe &&
                    (neighbor.mana_state == ManaState::None || neighbor.mana_state == ManaState::Light)
                ) {
                    neighbor.mana_state = ManaState::Light;
                    neighbor.is_powered = true;
                    break; // Output to one valid adjacent pipe per tick
                }
            }
        }
    }
};

} // namespace alx
