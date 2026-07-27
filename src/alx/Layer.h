#pragma once

namespace alx {
namespace Layer {

constexpr int Ground               = 0;   // Tile / floors
constexpr int GroundFixture        = 1;   // Pipe, Seep
constexpr int GroundFixtureItem    = 2;   // Dark mana, light mana liquid flow
constexpr int GroundFixtureItemFX  = 3;   // Light mana orb core / top FX
constexpr int GroundItem           = 4;   // AlloyItem nuggets
constexpr int WorldObj             = 10;  // Player, enemies, refiners, spires
constexpr int WorldOverlay         = 90;  // Twilight dark screen overlay
constexpr int HUD_BG               = 99;  // HUD background rects
constexpr int HUD_Text             = 100; // HUD text labels

} // namespace Layer
} // namespace alx
