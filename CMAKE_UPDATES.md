NOTE: switch CMake to use modern flags etc for better binary size compression options (but compare sizes, and see!), and also do for all platforms: win, macOS, linux! (we already started for win, but double check!)

---

"Modern CMake" (CMake 3.15+) replaced global string manipulation like `set(CMAKE_CXX_FLAGS_RELEASE ...)` with declarative, target-based, and build-type abstractions.

Instead of hacking low-level compiler flag strings (`/O1`, `/Ob1`, `-O3`), modern CMake gives you two high-level mechanisms:

---

### 1. Modern Optimization Flag Abstraction: `MSVC_RUNTIME_LIBRARY` & `CMAKE_MSVC_OPTIMIZATION_DATA`

Starting in **CMake 3.15+**, CMake introduced **`CMAKE_MSVC_OPTIMIZATION_DATA`** and standard optimization control properties.

In modern CMake, the standard way to enforce size optimization globally across all targets (without manually wiping flag strings) is setting **`CMAKE_OPTIMIZE_OPTION`** or configuring target-level options:

```cmake
# Tells CMake to optimize for MINIMUM SIZE across all Release configurations
# automatically mapping to /O1 /Os on MSVC, -Os on GCC/Clang
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)

```

However, for fine-grained MSVC control, modern CMake provides target property settings or clean per-target abstractions using `target_compile_options` and `target_link_options`.

---

### 2. Modern Target-Based Options: `target_compile_options` and `target_link_options`

Instead of altering global variables (`CMAKE_CXX_FLAGS_RELEASE`), Modern CMake attaches flags directly to specific targets using **Generator Expressions**:

```cmake
# ==========================================
# MODERN TARGET-BASED MSVC RELEASE OPTIONS
# ==========================================
if(MSVC)
    # Target-specific compiler options for Release configuration only
    target_compile_options(${GAME_BIN} PRIVATE
        $<$<CONFIG:Release>:
            /O1     # Minimize size
            /Os     # Favor small code
            /Ob1    # Inline expansion only
            /GF     # String pooling
            /Gy     # Function-level linking
            /Gw     # Global data optimization
            /GL     # Whole program optimization (LTCG)
        >
    )

    # Target-specific linker options for Release configuration only
    target_link_options(${GAME_BIN} PRIVATE
        $<$<CONFIG:Release>:
            /OPT:REF   # Eliminate unreferenced code/data
            /OPT:ICF   # Merge identical functions
            /LTCG      # Link-time code generation
        >
    )
endif()

```

---

### Why the Target-Based Approach is "Modern CMake" Best Practice:

1. **No String Wiping Hacks:** You don't have to erase internal CMake cache variables like `CMAKE_CXX_FLAGS_RELEASE` using `FORCE`.
2. **Encapsulation:** The options apply directly to your target `${GAME_BIN}` without leaking into third-party dependencies (like `MiniFB`), which might need their own flags.
3. **Generator Expressions (`$<$<CONFIG:Release>:...>`)**: Modern CMake uses generator expressions to apply options conditionally based on the active build configuration (Debug vs. Release) without polluting global state.