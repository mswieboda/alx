#pragma once

namespace alx {
namespace Layer {

constexpr int Ground               = 0;   // Tile / floors
constexpr int GroundFixture        = 1;   // Pipe, Seep
constexpr int GroundFixtureItem    = 2;   // Dark mana, light mana liquid flow
constexpr int GroundFixtureItemFX  = 3;   // Light mana orb core / top FX
constexpr int GroundItem           = 4;   // AlloyItem nuggets
constexpr int WorldObjBG           = 9;   // Background FX / Shadows behind world objects
constexpr int WorldObj             = 10;  // Player, enemies, refiners, spires, dark tower base
constexpr int WorldObjFX           = 16;  // Melee attack sword slash trails, blood sparks
constexpr int WorldObjSpireTop     = 19;  // Overhead Spire / Roof Peak lines (renders above player + FX when behind)
constexpr int WorldOverlay         = 90;  // Twilight dark screen overlay
constexpr int HUD_BG               = 99;  // HUD background rects
constexpr int HUD_Text             = 100; // HUD text labels
constexpr int HUD_Overlay          = 105; // on top of hud, like game over fade
constexpr int HUD_OverlayText      = 107; // on top of hud, like game over overlay text
constexpr int SceneFade            = 999; // Scene fade in/out transition

} // namespace Layer
} // namespace alx
