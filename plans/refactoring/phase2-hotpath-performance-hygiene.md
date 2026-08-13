# Phase 2 Plan: Hot-Path Memory & Performance Hygiene (`phase2-hotpath-performance-hygiene.md`)

## `[EP-HPRF]`: Executive Overview & Objectives

The primary objective of **Phase 2** is to eliminate runtime performance stutters, memory fragmentation, dynamic heap allocations in hot-path simulation/render loops, OS thread context-switching overhead, and $O(N)$ linear scans during high-frequency particle emission.

Resolving these issues will:
1. Guarantee zero dynamic heap allocations (`new`, `malloc`, `vector::push_back` resizes) inside `update()` or `render()` loops.
2. Eliminate audio stutter and thread synchronization overhead by moving SFX synthesis to a static voice pool.
3. Upgrade particle emission and active count queries from $O(N)$ linear scans to $O(1)$ constant-time operations.
4. Enforce strict clip rectangle bounds checking in software rendering passes to prevent out-of-bounds memory writes.

---

## Target Files & Code Locations

* [`src/core/Audio.cpp`](file:///Users/matt/code/cpp/alx/src/core/Audio.cpp#L42-L109) / [`src/core/Audio.h`](file:///Users/matt/code/cpp/alx/src/core/Audio.h)
* [`src/alx/Network.cpp`](file:///Users/matt/code/cpp/alx/src/alx/Network.cpp#L254-L322) / [`src/alx/Network.h`](file:///Users/matt/code/cpp/alx/src/alx/Network.h)
* [`src/alx/ParticleSystem.cpp`](file:///Users/matt/code/cpp/alx/src/alx/ParticleSystem.cpp#L11-L28) / [`src/alx/ParticleSystem.h`](file:///Users/matt/code/cpp/alx/src/alx/ParticleSystem.h#L13-L38)
* [`src/core/MiniFBWindow.cpp`](file:///Users/matt/code/cpp/alx/src/core/MiniFBWindow.cpp#L246-L262) / [`src/core/DrawPixels.cpp`](file:///Users/matt/code/cpp/alx/src/core/DrawPixels.cpp#L48-L341)

---

## `[PH-AUDO]`: Audio Subsystem Thread & Heap Allocation Removal (COMPLETED)

### Identified Anti-Patterns
* Calling `Audio::play_sfx()` spawns a detached OS thread (`std::thread(...).detach()`) and allocates a dynamic `std::vector<float>` heap buffer on *every single sound effect trigger*.
* Spawning OS threads on the audio hot-path causes context-switching overhead, thread safety hazards, and micro-stutters under heavy audio playback.

### Detailed Architecture & Data Structures
```cpp
struct SfxVoice {
    bool active{false};
    SfxrParams params{};
    float volume{0.0f};
    float pan{0.0f};
    
    uint32_t current_sample{0};
    uint32_t total_samples{0};
    float phase{0.0f};
    float f_freq{0.0f};
    float f_slide{0.0f};
    float vibrato_phase{0.0f};
    uint32_t noise_seed{0};
};

namespace AudioConfig {
    static constexpr size_t MAX_ACTIVE_VOICES     = 32; // can even set to 64 or higher
    static constexpr float FREQ_SCALE_HZ          = 8000.0f;
    static constexpr float SLIDE_SCALE            = 100.0f;
    static constexpr float VIBRATO_DEPTH_SCALE    = 100.0f;
    static constexpr float VIBRATO_SPEED_SCALE    = 0.05f;
    static constexpr float SFX_MASTER_GAIN        = 0.25f;
    static constexpr float PI                     = 3.14159265358979323846f;
}
```

### Action Plan & Sub-Tasks
- [x] `[AUPOL]`: Implement static `SfxVoice` struct and fixed voice pool `std::array<SfxVoice, AudioConfig::MAX_ACTIVE_VOICES> g_voices;` in [`src/core/Audio.cpp`](file:///Users/matt/code/cpp/alx/src/core/Audio.cpp).
- [x] `[AUPIV]`: Define `AudioConfig` `constexpr` constants in [`src/core/Audio.h`](file:///Users/matt/code/cpp/alx/src/core/Audio.h) replacing magic literals (`8000.0f`, `100.0f`, `3.14159265f`, etc.).
- [x] `[AUSTM]`: Refactor `play_sfx()` to eliminate `std::thread` spawning and buffer allocation, initializing an available slot in `g_voices` in $O(1)$ constant time.
- [x] `[AUVST]`: Implement deterministic voice stealing strategy when all voice slots are occupied.
- [x] `[AURTM]`: Update `audio_data_callback()` to perform real-time incremental SFXR sample synthesis directly during the miniaudio mix pass, removing `generate_sfx_buffer()`.
- [x] `[AUVRF]`: Verify syntax and compilation via `task build`, confirming zero heap allocations on audio triggers.

---

## `[PH-NETW]`: Network Simulation Hot-Path Allocation Buffer Pre-Allocation

### Identified Anti-Patterns
* `sim_tick()` and its helper methods (`compute_distance_field`, `downstream_dir`, `sim_pipe_flow`) allocate multiple `std::vector` and `std::queue` heap instances on every simulation frame (`std::vector<Fixture> next_fixtures`, `std::vector<int> dist`, `std::vector<DarkPipeIndex>`, `std::queue<int> q`).
* Heap allocations inside hot update loops trigger cache line misses, memory fragmentation, and frame drops.

### Action Plan & Sub-Tasks
* `[NETSC]`: Add reusable scratch buffers as private member variables in `Network.h`:
  ```cpp
  std::vector<Fixture> m_scratch_next_fixtures;
  std::vector<int> m_scratch_seep_dist;
  std::vector<int> m_scratch_spire_dist;
  std::vector<DarkPipeIndex> m_scratch_dark_pipes;
  std::vector<LightPipeIndex> m_scratch_light_pipes;
  std::vector<int> m_scratch_bfs_queue;
  ```
* `[NETRZ]`: Pre-allocate and resize scratch buffers only when `Network::resize()` is called.
* `[NETCL]`: Use `.clear()` (preserving buffer capacity) on scratch vectors during `sim_tick()` to achieve zero dynamic allocations on hot update paths.

---

## `[PH-PART]`: ParticleSystem $O(1)$ Free-List Index Pool Optimization (COMPLETED)

### Identified Anti-Patterns
* `ParticleSystem::emit()` performs an $O(N)$ linear scan over up to 1,024 pool slots looking for an inactive particle every single time `emit()` is called. When 50 particles are emitted in a single frame (e.g. `spawn_tower_shatter`), this executes up to 51,200 loop iterations per frame.
* `ParticleSystem::active_count()` performs a full $O(N)$ 1,024-element scan per call.

### Action Plan & Sub-Tasks
- [x] `[PARFL]`: Implement an $O(1)$ constant-time free-list index tracking array in `ParticleSystem`:
  ```cpp
  std::array<uint16_t, POOL_CAPACITY> m_free_slots;
  size_t m_free_count{POOL_CAPACITY};
  size_t m_active_count{0};
  ```
- [x] `[PAREM]`: Update `emit()` to pop an index from `m_free_slots` in $O(1)$ constant time. When a particle dies in `update()`, push its index back onto `m_free_slots` in $O(1)$ time.
- [x] `[PARAC]`: Return `m_active_count` directly in `active_count()` in $O(1)$ time without linear array iteration.

---

## `[PH-DRAW]`: Framebuffer Out-of-Bounds Protection & RenderTarget Context

### Identified Anti-Patterns
* In `MiniFBWindow::present()`, raw pixel indexing (`&m_presentation_pixel_buffer[target_y * window_w + px_start]`) lacks explicit clip boundary checks.
* Drawing routines in `DrawPixels.cpp` hardcode `Game::WIDTH` (320) and `Game::HEIGHT` (240) globally instead of querying target buffer parameters.

### Action Plan & Sub-Tasks
* `[DRWCP]`: Introduce explicit clip rectangle boundary checks in `MiniFBWindow::present()`:
  ```cpp
  if (target_x >= 0 && target_x < window_w && target_y >= 0 && target_y < window_h) {
      // Write pixel safely
  }
  ```
* `[DRWRT]`: Introduce a explicit `RenderTarget` context struct in `DrawPixels.h`:
  ```cpp
  struct RenderTarget {
      std::span<uint32_t> pixels;
      int width{320};
      int height{240};
  };
  ```
* `[DRWFX]`: Update `DrawPixels` primitive routines (`rect`, `oval`, `line`, `sprite_frame`) to draw into `RenderTarget` bounds rather than assuming fixed global screen dimensions.

---

## Verification & Build Criteria

After completing Phase 2 edits:
1. Run syntax and type-safety verification:
   ```bash
   task build
   ```
2. Verify zero dynamic heap allocations in hot paths by inspecting `sim_tick()`, `Audio::play_sfx()`, and `ParticleSystem::emit()`.
3. Verify $O(1)$ particle allocation behavior under heavy particle bursts (`spawn_tower_shatter`).
