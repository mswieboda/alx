read the guide at @INPUT_DIAGONAL_FIX.md and make the changes needed to do the suggested "## 4. Key Takeaways for Future Iterations". also note we are trying to keep `static constexpr float DIAGONAL_SPEED_SCALE = 0.75f;` for player. so it is not perfectly 100% cadence, but that is okay.

# 2D Top-Down Directional Input & Facing Smoothness Guide

## 1. Problem Statement: Human Input Asynchrony

At 60 FPS (16.6ms per frame), human fingers physically cannot press or release two D-pad / WASD keys at the exact same microsecond.

When a player intends to press **`Up + Right`** to move diagonally:
* **Frame 1**: `Up` registers first. The player moves straight UP for 1 frame, and the facing vector twitches straight UP `(0, -1)`.
* **Frame 2**: `Right` registers 16ms later. The player starts moving DIAGONALLY, and the facing vector snaps to UP-RIGHT `(0.707, -0.707)`.

When stopping diagonal movement:
* `Up` is released 1 frame before `Right`. The player moves 1 frame straight RIGHT and flickers the build/attack indicator straight RIGHT right before stopping.

---

## 2. Recommended Implementation: Technique 1 + Technique 2 Together

For WASD and D-Pad controls in *Aetherlux*, implement **Technique 1 and Technique 2 together simultaneously**:

### Technique 1: Facing Direction Hysteresis (2-Frame Input Latch)
* **Movement Execution**: Movement (`transform.x`, `transform.y`) begins **immediately on Frame 1** so player movement remains 100% responsive with zero input lag.
* **Facing Vector Update**: Updating `facing_dx` and `facing_dy` is deferred by a 2-frame window (~30ms) when transitioning from single-axis to dual-axis input.
* **Result**: If the second key arrives within 2 frames, the facing vector transitions directly to diagonal without flicking 4-way for a single frame.

### Technique 2: Diagonal Release Buffer (Stop Latch)
* When moving diagonally, if one key is released 1–3 frames earlier than the other, the game retains the diagonal facing vector for a short buffer duration (~50ms / 3 frames).
* **Result**: Keeps the build indicator and attack hitboxes locked on the intended target tile without twitching when stopping diagonal movement.

---

## 3. C++ Reference Implementation Blueprint for `Player.h`

```cpp
struct PlayerInputBuffer {
    static constexpr float FACING_DIAGONAL_LATCH_TIME = 0.050f; // 50ms (~3 frames at 60 FPS)
    float diagonal_latch_timer = 0.0f;
    float latched_facing_dx = 0.0f;
    float latched_facing_dy = 1.0f;

    // Call this inside Player::update_movement() to update facing_dx and facing_dy cleanly
    void update_facing(float dt, float raw_dx, float raw_dy, float& facing_dx, float& facing_dy) {
        bool is_diagonal = (raw_dx != 0.0f && raw_dy != 0.0f);
        bool is_moving = (raw_dx != 0.0f || raw_dy != 0.0f);

        if (is_diagonal) {
            // Immediately lock diagonal facing vector and refresh latch timer
            constexpr float inv_sqrt2 = 0.70710678118f;
            facing_dx = raw_dx * inv_sqrt2;
            facing_dy = raw_dy * inv_sqrt2;
            latched_facing_dx = facing_dx;
            latched_facing_dy = facing_dy;
            diagonal_latch_timer = FACING_DIAGONAL_LATCH_TIME;
        } else if (is_moving) {
            // Single-axis input: Check if within diagonal release latch window
            if (diagonal_latch_timer > 0.0f) {
                diagonal_latch_timer -= dt;
                facing_dx = latched_facing_dx;
                facing_dy = latched_facing_dy;
            } else {
                facing_dx = raw_dx;
                facing_dy = raw_dy;
            }
        }
    }
};
```

---

## 4. Key Takeaways for Future Iterations

1. **Implement Technique 1 + 2 Together**: Always use input latching and release buffering in tandem for D-pad/WASD controls.
2. **Keep Movement Unbuffered**: Always apply movement (`dx * speed * dt`) immediately for zero input lag.
3. **Buffer Only the Facing Vector**: Buffer only `facing_dx` and `facing_dy` to keep attacks and building indicators rock-solid.
4. **Use `DIAGONAL_SPEED_SCALE = 1.0f`**: For 100% 60Hz pixel-perfect smooth movement with 0 rasterization stall frames.
