Here is the structured integration plan to pull `minigamepad` into your repository, configure CMake, update documentation, and hook digital gamepad events straight into your `core/Input.h` and `alx/Action.h` architecture.

---

## Part 1: Dependency Setup & Documentation Plan

### 1. Git Submodule Setup

Add `minigamepad` as a submodule pointing to `./deps/minigamepad`.

```bash
# Add submodule into deps folder
git submodule add https://github.com/dpethes/minigamepad.git deps/minigamepad

```

---

### 2. CMake Integration (`CMakeLists.txt`)

Since `minigamepad` is a C header library with platform-specific backend implementations, update `CMakeLists.txt` to include its directory and link required system frameworks/libraries depending on the target platform.

```cmake
# --- Dependencies ---
add_subdirectory(deps/minifb)

# Include minigamepad directory so #include "minigamepad.h" works
include_directories(deps/minigamepad)

# (AI request NOTE: PROBABLY ALREADY IN CMakeLists.txt somewhere)
# Build project executable
# add_executable(${PROJECT_NAME} ${SOURCES})

# (AI request NOTE: PROBABLY ALREADY IN CMakeLists.txt somewhere)
# Target link libraries (Platform OS additions for Gamepad API support)
# target_link_libraries(${PROJECT_NAME} PRIVATE minifb)

# Required libs for minigamepad
if(APPLE)
    # macOS GameController and IOKit frameworks needed by minigamepad
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework GameController"
        "-framework IOKit"
        "-framework Foundation"
    )
elseif(WIN32)
    # Windows XInput / DirectInput support dependencies
    target_link_libraries(${PROJECT_NAME} PRIVATE xinput hid setupapi)
elseif(UNIX AND NOT APPLE)
    # Linux evdev / udev support dependencies
    target_link_libraries(${PROJECT_NAME} PRIVATE udev)
endif()

```

---

### 3. Documentation (`README.md`)

Update the project `README.md` to note the new dependency and explain submodule initialization:

````markdown
## Building & Setup

### Prerequisites & Dependencies
* `minifb` (Software framebuffer and window management)
* (AI request: PLEASE CHANGE THIS: LIST THE OTHERS in `./deps` )
* `minigamepad` (Lightweight digital gamepad input polling)

### Cloning with Submodules
```bash
git clone --recursive <repository-url>
# OR if already cloned:
git submodule update --init --recursive

```

### Platform Requirements

* **macOS:** Requires macOS SDK with `GameController.framework`.
* **Windows:** Standard `XInput` libraries (included in Windows SDK).
* **Linux:** Requires `libudev-dev` installed (`sudo apt install libudev-dev`).

````

---

## Part 2: Code Integration Plan (Pseudo C++ Code)

*Note: The following code is **pseudo C++ code** structured around typical GBA-style digital input needs. It outlines where and how to initialize `mgp`, poll state, and feed digital buttons directly into your `core/Input.h` and `alx/Action.h` map. But it is not using known files from alx codebase, so find where these need to go, or need to be tweaked/modifed!*

### 1. `minigamepad` Lifecycle Management
Initialize `minigamepad` right after window creation and clean it up before exit.

```cpp
// Pseudo C++ Code - Main Initialization / Shutdown Setup

#include <minifb.h>

// MINIGAMEPAD Implementation define in ONE .cpp file
#define MINIGAMEPAD_IMPLEMENTATION
#include "minigamepad.h"

int main() {
    // 1. Initialize MiniFB Window
    struct mfb_window *window = mfb_open_target("Aetherlux", 240, 160, MFB_ACCEL_AUTO);

    // 2. Initialize MiniGamepad right after window setup
    mgp_init();

    // Game loop
    while (running) {
        // ... (See Polling step below) ...
    }

    // 3. Shutdown MiniGamepad on cleanup
    mgp_shutdown();
    mfb_close(window);
    return 0;
}

```

---

### 2. Frame Input Polling & Mapping to `alx/Action.h`

`minigamepad` requires per-frame updates via `mgp_update()`. Poll digital state and translate button presses into `Action::XYZ` inputs inside your frame update cycle.

```cpp
// Pseudo C++ Code - Game Loop Input Translation

#include "core/Input.h"
#include "alx/Action.h"

void process_gamepad_inputs(core::InputState& inputState) {
    // Poll active gamepads
    mgp_update();

    // Check primary active controller (index 0)
    mgp_gamepad gamepad;
    if (mgp_get_gamepad(0, &gamepad) && gamepad.connected) {

        // --- Digital D-Pad Mapping ---
        if (gamepad.buttons & MGP_BUTTON_DPAD_UP)    inputState.set_action_active(alx::Action::MoveUp, true);
        if (gamepad.buttons & MGP_BUTTON_DPAD_DOWN)  inputState.set_action_active(alx::Action::MoveDown, true);
        if (gamepad.buttons & MGP_BUTTON_DPAD_LEFT)  inputState.set_action_active(alx::Action::MoveLeft, true);
        if (gamepad.buttons & MGP_BUTTON_DPAD_RIGHT) inputState.set_action_active(alx::Action::MoveRight, true);

        // --- GBA Face & Shoulder Button Mapping ---
        // GBA Action / Primary Attack -> Button A
        if (gamepad.buttons & MGP_BUTTON_A) {
            inputState.set_action_active(alx::Action::Attack, true);
        }

        // GBA Secondary / Cancel / Roll -> Button B
        if (gamepad.buttons & MGP_BUTTON_B) {
            inputState.set_action_active(alx::Action::Special, true);
        }

        // GBA Shoulders (L / R)
        if (gamepad.buttons & MGP_BUTTON_LB) inputState.set_action_active(alx::Action::PrevItem, true);
        if (gamepad.buttons & MGP_BUTTON_RB) inputState.set_action_active(alx::Action::NextItem, true);

        // --- Start & Select ---
        if (gamepad.buttons & MGP_BUTTON_START)  inputState.set_action_active(alx::Action::Pause, true);
        if (gamepad.buttons & MGP_BUTTON_SELECT) inputState.set_action_active(alx::Action::MapOrInventory, true);
    }
}

```

---

## Part 3: Key Integration Notes & Considerations

* **`MINIGAMEPAD_IMPLEMENTATION` Placement:** Ensure the `#define MINIGAMEPAD_IMPLEMENTATION` guard is placed in **only one** translation unit (`.cpp` file) in your project (e.g., `src/core/Input.cpp` or `src/main.cpp`) to avoid duplicate symbol linker errors.
* **Input Unioning (Keyboard + Gamepad):** Structure `core/Input.h` so gamepad actions **OR** into keyboard actions rather than overwriting them. This allows seamless hot-swapping between keyboard and controller during gameplay.
* **Hot-plugging Support:** `mgp_update()` handles hot-plugging automatically. Checking `gamepad.connected` every frame ensures no crashes or invalid reads occur if a controller is unplugged mid-game jam session!
