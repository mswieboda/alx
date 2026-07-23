# Dark Mana Corner Rendering — Remaining Fixes

## Context

The corner L-bend rendering now correctly:
- Detects corners on first encounter (via `get_downstream_dir()` fallback)
- Grows the incoming arm from the entry edge toward the hub (progress 0→0.5)
- Grows the outgoing arm from the hub toward the exit edge (progress 0.5→1.0)
- Backed-up pipes render as solid static fills (no flashing)

Two visual artifacts remain after the slug finishes traversing a corner tile.

---

## Fix 1: Corner exit — shrink incoming arm after slug passes through

**Problem**: Once the slug finishes flowing through the corner (progress → 1.0), the full L-shape stays rendered (both arms at 16px). On the next tick the corner tile is vacated and the L disappears instantly. Worse, the full incoming arm overlaps into visual space outside the pipe casing at the entry edge after the slug has conceptually left.

**Expected behavior**: After the slug head exits the corner tile (progress 0.5→1.0), the incoming arm should shrink from the entry edge back toward the hub — the reverse of how it entered. The slug "drains out" of the entry side as it flows into the exit side.

**Implementation sketch** in `draw_tile_mana()` corner branch:
```
// Slug head travels L-path: entry_edge → hub (half=16) → exit_edge (half=16)
// Total path = tile_size (32). Head pos = progress * tile_size.
//
// Incoming arm: grows from edge→hub during first half, then shrinks hub→edge
//   in_len = (head_s <= half)
//          ? head_s                          // growing: 0 → 16
//          : half - (head_s - half)          // shrinking: 16 → 0
//         = (head_s <= half) ? head_s : (tile_size - head_s)
//
// Outgoing arm: grows from hub→edge during second half
//   out_len = max(0, head_s - half)          // 0 → 16
```

This keeps the slug at a constant apparent length (~16px visible at any time) rather than accumulating into a full L.

---

## Fix 2: Previous pipe shortens in sync with corner entry

**Problem**: When a slug enters a corner tile, the source straight pipe is vacated instantly (mana_state = None). Its rendering disappears at the tick boundary while the corner's incoming arm grows from 0. This creates a brief visual gap — the slug "pops" from one tile to the next rather than sliding continuously.

**Expected behavior**: The straight pipe before the corner should shorten its slug from the downstream end (toward the corner) as the corner's incoming arm grows from the entry edge. The two visuals stay connected: source pipe tail recedes while corner arm advances.

**Implementation sketch**:

This requires the source tile to still render a partial slug even though its `mana_state` is `None`. Options:

1. **Rendering-side peek at downstream tile**: In the straight-pipe rendering path, after drawing the full slug at `render_x/render_y`, check if the downstream neighbor is a corner tile that is actively receiving mana this tick. If so, shorten the slug from the downstream end by `in_len` pixels (the corner's incoming arm length). This keeps the two visuals stitched together.

2. **Ghost rendering via move vector**: When a tile is vacated (`mana_state = None`) but its `out_dx/out_dy` still point to a downstream corner tile, render a shrinking "ghost" slug for that one tick. The ghost length = `tile_size - (progress * tile_size)`, shrinking from full to zero as the corner's incoming arm grows from zero to full.

Option 1 is simpler since it doesn't require rendering vacated tiles — it adjusts the existing slug on the source tile before it disappears. However, the source tile is already vacated by the simulation, so option 2 (or a hybrid) may be needed.

**Complexity note**: This crosses tile boundaries in the renderer and requires coordinating two tiles' animations. Consider adding a `prev_move_dx/prev_move_dy` or `ghost_tick` field to `Tile` if the rendering-only approach proves insufficient.
