Here is the detailed, phased implementation plan for **Software Particle System** in *Aetherlux*.

This plan is structured to execute each sub-task independently, run build verification, and move incrementally without breaking existing engine loops.

---

# 📜 Master Implementation Roadmap: Particle System

```
  Phase 1: Particle Engine Core & Fixed Pool Infrastructure
       │
       ▼
  Phase 2: Generic Emitters & Preset Behaviors (Sparks & Embers)
       │
       ▼
  Phase 3: Dark Mana Flow ── Straight Pipe Segments (Parametric Linear)
       │
       ▼
  Phase 4: Dark Mana Flow ── Curved Pipe Intersections (Quadratic Bezier)
       │
       ▼
  Phase 5: Backed-Up Pipes & Dead-End Freeze Logic (Strategy 2)
       │
       ▼
  Phase 6: Network Integration, Emitter Hookups & Optimization

```

---

## Phase 1: Core Particle Engine Infrastructure

### Subtask 1.1: Data Layout (`Particle.h`) [COMPLETED]

* **Objective:** Define a compact, cache-aligned struct that supports both Kinematic Physics (Route A) and Parametric Pathing (Route B) without dynamic allocations.
* **Algorithm / Data Fields:**
* `x, y`: Current floating-point world position.
* `render_x, render_y`: Screen-space rendering position (accounts for pressure jitter when frozen).
* `vx, vy`: Kinematic velocities (used for Route A particles).
* `start_x, start_y`: Origin coordinates of the parametric trajectory (Route B).
* `target_x, target_y`: Destination coordinates of the parametric segment (Route B).
* `control_x, control_y`: Midpoint control node for curved Bezier paths (Route B).
* `life, max_life`: Floating-point timers in seconds ($t = 1.0 - [\text{life} / \text{max\_life}]$).
* `param_a`: Generic float parameter (used for ripple amplitude, wobble frequency, or jitter intensity).
* `color`: Packed 32-bit RGBA color (`0xAARRGGBB`).
* `size`: Unsigned 8-bit integer (1 = single pixel, 2 = $2\times2$ rect, $r = 3+$ = circle primitive).
* `tile_x, tile_y`: Unsigned 16-bit grid coordinates (used for checking local pipe network flow states).
* `type`: Enum (`Spark`, `LightEmber`, `ManaPulseStraight`, `ManaPulseCurved`).
* `active`: Boolean flag indicating slot availability.



### Subtask 1.2: Fixed Pool Manager (`ParticleSystem.h`)

* **Objective:** Create a zero-heap-allocation manager class using a fixed array.
* **Data Structure:** `std::array<Particle, 256> m_pool;`
* **Allocation Strategy:** Ring-buffer fallback pointer (`m_next_slot`). When emitting, search for the first `active == false` slot. If all 256 slots are busy, overwrite `m_pool[m_next_slot]` and increment `m_next_slot = (m_next_slot + 1) % 256` to ensure new gameplay juice never stalls.

### Subtask 1.3: Main Update & Deferred Render Command Pass

* **Objective:** Update lifetimes and enqueue draw calls to your `Draw` command queue.
* **Algorithms:**
* **Life Decay:** Subtract delta time (`life -= dt`). Deactivate if `life <= 0.0f`.
* **Alpha Falloff Calculation:**

$$\text{alpha\_byte} = \text{static\_cast}<\text{uint8\_t}>((\text{life} / \text{max\_life}) \times 255.0\text{f})$$



Re-pack top 8 bits of color with calculated alpha byte.
* **Screen Space Transformation:** Subtract camera world offsets (`screen_x = render_x - cam_x`).
* **Deferred Queue Insertion:** Enqueue pixel or circle primitive commands at $Z\text{-index} = 8000$ (renders above ground fixtures, below HUD).



---

## Phase 2: Generic Emitters & Preset Behaviors

### Subtask 2.1: Kinematic Physics Loop (Route A)

* **Objective:** Implement physics-driven particles for combat impacts and ambient spires.
* **Algorithms:**
* **Sword Sparks (`ParticleType::Spark`):**
* Update: Apply velocity drag $\vec{V} = \vec{V} \times (0.90)^{\text{dt\_scale}}$.
* Motion: $\vec{P} = \vec{P} + \vec{V} \cdot dt$.


* **Light Spire Embers (`ParticleType::LightEmber`):**
* Update: Upward buoyancy ($v_y -= 15.0 \cdot dt$) and random lateral wobble ($v_x += \text{rand\_range}(-10, 10) \cdot dt$).
* Motion: $\vec{P} = \vec{P} + \vec{V} \cdot dt$.





### Subtask 2.2: Combat & Spire Helper Emitters

* **Objective:** Expose clean trigger functions for `MainScene`.
* **Sub-Steps:**
* `spawn_hit_sparks(float x, float y, int count)`: Spawns a radial burst of 8–12 bright amber/gold sparks upon sword-to-enemy collision.
* `spawn_spire_embers(float x, float y)`: Periodically called by active Light Spires to float cyan light particles upward into the room.



---

## Phase 3: Dark Mana Flow — Straight Pipe Segments

### Subtask 3.1: Straight Segment Emitter Math

* **Objective:** Calculate precise tile-to-tile vectors based on pipe direction metadata (`PipeDir::North`, `South`, `East`, `West`).
* **Algorithm:**
1. Determine tile origin $(T_x, T_y)$ and directional unit vector $\vec{D}$.
2. Compute segment start point $P_0 = (T_x \cdot 16 + 8, T_y \cdot 16 + 8) - \vec{D} \cdot 8$.
3. Compute segment target point $P_1 = (T_x \cdot 16 + 8, T_y \cdot 16 + 8) + \vec{D} \cdot 8$.
4. Generate a perpendicular offset orthogonal to $\vec{D}$ (range $\pm 2$ to $\pm 4$ pixels) to fill out the inner pipe width.
5. Assign `start` and `target` coordinates to the particle.



### Subtask 3.2: Linear Parametric Motion (Route B - Linear)

* **Objective:** Move straight pipe particles smoothly from entry to exit.
* **Algorithm:**
* Compute normalized progress progress $t \in [0.0, 1.0]$:

$$t = 1.0 - \left(\frac{\text{life}}{\text{max\_life}}\right)$$


* Interpolate base linear position:

$$x_{\text{base}} = \text{start\_x} + (\text{target\_x} - \text{start\_x}) \cdot t$$


$$y_{\text{base}} = \text{start\_y} + (\text{target\_y} - \text{start\_y}) \cdot t$$





### Subtask 3.3: Viscous Perpendicular Ripple Modulation

* **Objective:** Layer procedural fluid physics over the linear line to give dark mana a lumpy, moving fluid appearance.
* **Algorithm:**
* Calculate wave offset: $\text{ripple} = \sin(t \cdot \text{frequency}) \cdot \text{amplitude}$.
* Apply offset orthogonally to the travel vector (e.g., if traveling horizontally along $X$, modulate $Y$ by $\text{ripple}$).
* Store final values in `x, y` and set `render_x = x, render_y = y`.



---

## Phase 4: Dark Mana Flow — Curved Pipe Intersections

### Subtask 4.1: Corner Intersection Emitter Classification

* **Objective:** Detect when a pipe fixture represents an elbow/turn (e.g., fluid entering from South and exiting East).
* **Algorithm:**
* Identify entry vector $\vec{V}_{\text{in}}$ and exit vector $\vec{V}_{\text{out}}$.
* Set $P_0$ at the entry tile edge along $\vec{V}_{\text{in}}$.
* Set $P_1$ at the exit tile edge along $\vec{V}_{\text{out}}$.
* Set the Control Node $P_{\text{ctrl}}$ at the intersection vertex (tile center or inner elbow corner).



### Subtask 4.2: Quadratic Bezier Curve Motion

* **Objective:** Drive the particle trajectory along a smooth 90-degree curve using parametric Bezier math.
* **Mathematical Formula:**
Given progress $t = 1.0 - (\text{life} / \text{max\_life})$ and $u = 1.0 - t$:

$$B(t) = u^2 \cdot P_0 + 2u \cdot t \cdot P_{\text{ctrl}} + t^2 \cdot P_1$$


* **Implementation:**
* $x = u^2 \cdot \text{start\_x} + 2u \cdot t \cdot \text{control\_x} + t^2 \cdot \text{target\_x}$
* $y = u^2 \cdot \text{start\_y} + 2u \cdot t \cdot \text{control\_y} + t^2 \cdot \text{target\_y}$



### Subtask 4.3: Radial Curve Width Offset

* **Objective:** Ensure particles stay within the 4–6px inner pipe walls during sharp turns without clipping the corners.
* **Algorithm:** Scale lateral offset by $\sin(\pi \cdot t)$ so maximum perpendicular width occurs midway through the turn, tapering at entry and exit points.

---

## Phase 5: Backed-Up Pipes & Dead-End Freeze Logic (Strategy 2)

### Subtask 5.1: Network State Query Integration

* **Objective:** Allow the central particle loop to query local tile flow states.
* **Interface update:** Modify update signature to `ParticleSystem::update(float dt, const Network& network, float global_time)`.

### Subtask 5.2: TTL Countdown Suspension (Particle Freeze)

* **Objective:** Pause progress $t$ when fluid encounters a dead-end or backed-up segment.
* **Algorithm:**
1. For particles of type `ManaPulseStraight` or `ManaPulseCurved`, lookup `tile_x, tile_y` in `network`.
2. Query tile flow condition: `bool is_blocked = network.is_pipe_blocked_at(p.tile_x, p.tile_y);`
3. **If `is_blocked == true`:**
* **DO NOT** execute `p.life -= dt;` (TTL timer freezes, locking progress $t$).
* Skip standard trajectory updates ($x, y$ coordinates remain locked in place).





### Subtask 5.3: Pressure Jitter & Stagnant Fluid Visuals

* **Objective:** Give frozen particles a pulsating, high-pressure jitter effect to visually indicate a clogged or backed-up pipe.
* **Algorithm:**
* Calculate high-frequency jitter using global scene time:

$$\text{jitter\_x} = \sin(\text{global\_time} \cdot 18.0\text{f} + \text{p.start\_x}) \cdot 0.75\text{f}$$


$$\text{jitter\_y} = \cos(\text{global\_time} \cdot 18.0\text{f} + \text{p.start\_y}) \cdot 0.75\text{f}$$


* Assign rendering position: `render_x = x + jitter_x`, `render_y = y + jitter_y`.
* The particle remains frozen in mid-transit, vibrating with pressure until the downstream network block is cleared!



---

## Phase 6: Network Integration, Emitter Hookups & Optimization

### Subtask 6.1: Emitter Pulse Synchronization

* **Objective:** Tie particle generation directly to the `Network` BFS fluid update tick rather than spawning randomly every frame.
* **Logic:** When the Network updates active flow paths (e.g., every 0.2 seconds), iterate over all active pipe fixtures and trigger 1–2 fluid emissions per flowing tile.

### Subtask 6.2: Viewport Camera Culling

* **Objective:** Preserve performance by suppressing calculations for off-screen particles.
* **Logic:** Prior to running expensive Bezier math or drawing commands, evaluate:

$$\text{if } (x < \text{cam\_x} - 16 \text{ \vert{}\vert{} } x > \text{cam\_x} + \text{SCREEN\_W} + 16 \text{ \vert{}\vert{} } \dots) \Rightarrow \text{skip render}$$



### Subtask 6.3: Verification & Contingency Plan

* **Verification Checks:**
1. Confirm binary size increase remains under 2KB.
2. Verify zero dynamic memory allocations occur in `m_pool` under full load (256 active particles).
3. Validate that clearing a pipe blockage instantly resumes particle TTL countdowns and fluid motion.


* **Contingency Stash Plan:**
If network state query syntax becomes overly complex during Phase 5, git stash the branch (`git checkout -b feature/particles`) and transition to constructing the **Dark Tower static building fixture**.

---
