# Multi-Tile Fixture Buildings & Connection Ports Roadmap

## [EP-FTMB]: Multi-Tile Fixture Buildings & Port Network Epic
This epic refines multi-tile fixture building footprints (**Refiner: 3x3**, **Spire: 2x3**, **Seep: 3x2**), fixes world collision and player self-placement rules, removes x-ray transparency fading, implements strict perimeter connection ports, and updates 2.5D roof skylight frame rendering.

---

## [PH-SKYL]: Phase 5: 2.5D Roof Skylight Frame & Layered Interior Rendering (COMPLETED)

### [SPSR]: 1. Spire Vertical Crystal Chamber Assembly (2x3 Tiles / 32x48 px)
- `[SPGE]`: Chamber Cutout Geometry - Define a centered 16x16 px interior skylight cutout at `(world_x + 8, world_y + 16)`.
- `[SPFR]`: Emerald Pillar Frame - Render dual 4px vertical side pillars (`world_x + 4` and `world_x + 24`) and top/bottom bevel borders framing the inner crystal chamber.

### [LAYR]: 2. Strict 5-Stage Inset Interior Layering Order
- `[L1BG]`: Stage 1: Recess Pit Floor BG - Render deep dark floor background rect (`0xFF120A2A` Refiner, `0xFF002810` Spire) inside cutout window bounds.
- `[L2MN]`: Stage 2: Static Flat Mana Pool - Render solid dark mana pool (`0xFF4A0088` pipe dark mana base) or light mana cyan aura (`0xFF00FFFF`) + white core (`0xFFFFFFFF`) with static flat fluid rects matching current mana state.
- `[L3EM]`: Stage 3: Interior Emitter Embers - Spawn embers (`spawn_refiner_embers` / `spawn_spire_embers`) inside interior window bounds before roof frame draw.
- `[L4BV]`: Stage 4: Roof Frame & Bevel Edge Shadows - Render outer frame walls, side pillars, front face dual $6 \times 6\text{ px}$ windows showing dark mana, and 1px inner bevel edge shadow rects over liquid pool edges for inset depth.
- `[L5HL]`: Stage 5: Top Cap Highlights & Roof Vent Vanes - Draw top roof cap highlights (`0xFF7B4CE3` Refiner, `0xFF88FFCC` Spire tip) and roof vent vanes (`0xFF241454`).

### [CLPAL]: 3. High-Contrast Twilight Color Tokens & Shading Specs
- `[TBDCS]`: NOTE: to-be-determined on these colors, they are likely way too bright. and i might change my mind on the green or pink-ish colors as they were placeholders - NOT the actual end-game color scheme!
- `[RFCLR]`: Refiner Palette Specs - Body: `0xFF341C66`, Foundation Base: `0xFF1F1240`, Pit Floor: `0xFF120A2A`, Pipe Dark Mana Base: `0xFF4A0088`.
- `[SPCLR]`: Spire Palette Specs - Spire Tip: `0xFF88FFCC`, Crystal Peak: `0xFF00FF88`, Shaft: `0xFF00A350`, Foundation Base: `0xFF004520`, Pit Floor: `0xFF002810`.
