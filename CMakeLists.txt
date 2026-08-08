cmake_minimum_required(VERSION 3.25...4.3)

# CMake 3.25+ / 4.x Global Build Optimizations
set(CMAKE_OPTIMIZE_DEPENDENCIES ON)
if(POLICY CMP0155)
    # Disable C++20 module scanning when compiling standard header includes
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif()

# Global MSVC Runtime Library abstraction (Static CRT: MT for Release, MTd for Debug)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Initialize project name dynamically from argument
if(NOT GAME_BIN)
    set(GAME_BIN "alx")
endif()

project(${GAME_BIN} C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Interprocedural Optimization (LTO / LTCG) for Release builds
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

# ----------------------------------------------------

include_directories(deps)
include_directories(src)
include_directories(deps/miniaudio)
include_directories(deps/pocketmod)
include_directories(deps/minigamepad)

set(MINIFB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MINIFB_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(deps/minifb)

# Clear MSVC debug info format property on minifb targets so CMake doesn't
# inject /Zi alongside minifb's built-in /ZI Debug compile flag.
if(MSVC)
    foreach(tgt minifb minifb_version_info)
        if(TARGET ${tgt})
            set_property(TARGET ${tgt} PROPERTY MSVC_DEBUG_INFORMATION_FORMAT "")
        endif()
    endforeach()
endif()

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE C_SOURCES CONFIGURE_DEPENDS "src/*.c")
file(GLOB_RECURSE HEADER_SOURCES CONFIGURE_DEPENDS "src/*.h" "src/*.hpp")

add_executable(
    ${GAME_BIN}
    ${CPP_SOURCES}
    ${C_SOURCES}
    ${HEADER_SOURCES}
)

# ----------------------------------------------------
# Build Profiles Configuration (Debug vs Release)
# ----------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEV_TOOLS_DEFAULT ON)
else()
    set(DEV_TOOLS_DEFAULT OFF)
endif()

# Master toggle for development/debug tools
if(NOT DEFINED ALX_ENABLE_DEV_TOOLS)
    set(ALX_ENABLE_DEV_TOOLS ${DEV_TOOLS_DEFAULT})
endif()

# Sub-feature toggles cascade from ALX_ENABLE_DEV_TOOLS unless explicitly overridden
if(NOT DEFINED ALX_ENABLE_DEBUG)
    set(ALX_ENABLE_DEBUG ${ALX_ENABLE_DEV_TOOLS})
endif()

if(NOT DEFINED ALX_ENABLE_TELEMETRY)
    set(ALX_ENABLE_TELEMETRY ${ALX_ENABLE_DEV_TOOLS})
endif()

if(NOT DEFINED ALX_ENABLE_HEADLESS)
    set(ALX_ENABLE_HEADLESS ${ALX_ENABLE_DEV_TOOLS})
endif()

target_compile_definitions(${GAME_BIN} PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    ALX_ENABLE_DEV_TOOLS=$<BOOL:${ALX_ENABLE_DEV_TOOLS}>
    ALX_ENABLE_DEBUG=$<BOOL:${ALX_ENABLE_DEBUG}>
    ALX_ENABLE_TELEMETRY=$<BOOL:${ALX_ENABLE_TELEMETRY}>
    ALX_ENABLE_HEADLESS=$<BOOL:${ALX_ENABLE_HEADLESS}>
)

if(MSVC)
    # MSVC Compiler Options (Release size optimizations; Debug uses CMake defaults)
    target_compile_options(${GAME_BIN} PRIVATE
        $<$<CONFIG:Release>:
            /O1         # Minimize size
            /Os         # Favor small code
            /Ob1        # Inline expansion only
            /GF         # String pooling
            /Gy         # Function-level linking
            /Gw         # Global data optimization
            /GL         # Whole program optimization (LTCG)
            /GR-        # Disable RTTI
            /EHs-c-     # Disable exceptions
        >
    )

    # MSVC Linker Options
    target_link_options(${GAME_BIN} PRIVATE
        $<$<CONFIG:Release>:
            /OPT:REF   # Eliminate unreferenced code/data
            /OPT:ICF   # Merge identical functions
            /LTCG      # Link-time code generation
        >
    )
else()
    # GCC / CLANG (macOS & Linux) Compiler Options
    target_compile_options(${GAME_BIN} PRIVATE
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:Release>:
            $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Oz>
            $<$<CXX_COMPILER_ID:GNU>:-Os>
            -ffunction-sections
            -fdata-sections
            -fno-exceptions
            -fno-rtti
            -fno-asynchronous-unwind-tables
            -fvisibility=hidden
            -fvisibility-inlines-hidden
        >
    )

    # GCC / CLANG Linker Options
    if(APPLE)
        target_link_options(${GAME_BIN} PRIVATE
            $<$<CONFIG:Release>:
                -Wl,-dead_strip
                -Wl,-x
            >
        )
    elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
        target_link_options(${GAME_BIN} PRIVATE
            $<$<CONFIG:Release>:
                -Wl,--gc-sections
                -Wl,--as-needed
                -Wl,-s
            >
        )
    endif()
endif()

# Also apply size optimizations to MiniFB target if built
if(TARGET minifb AND NOT MSVC)
    target_compile_options(minifb PRIVATE
        $<$<CONFIG:Release>:
            $<$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>>:-Oz>
            $<$<C_COMPILER_ID:GNU>:-Os>
            -ffunction-sections
            -fdata-sections
            -fno-exceptions
            -fno-asynchronous-unwind-tables
            -fvisibility=hidden
        >
    )
endif()

target_link_libraries(${GAME_BIN} PRIVATE minifb)

# Platform-specific system dependencies
if(APPLE)
    target_link_libraries(${GAME_BIN} PRIVATE
        "-framework GameController"
        "-framework IOKit"
        "-framework Foundation"
        "-framework CoreFoundation"
    )
elseif(WIN32)
    # Required Windows system libraries for MiniFB, MiniAudio & MiniGamepad
    target_link_libraries(${GAME_BIN} PRIVATE user32 gdi32 shell32 winmm dwmapi opengl32 xinput hid setupapi)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(X11 REQUIRED)
    target_link_libraries(${GAME_BIN} PRIVATE X11::X11 pthread GL udev)
endif()

# Only strip and pack if we are building in Release mode
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(APPLE)
        # macOS Mach-O stripping
        add_custom_command(TARGET ${GAME_BIN} POST_BUILD
            COMMAND ${CMAKE_STRIP} -u -r $<TARGET_FILE:${GAME_BIN}>
            COMMENT "Stripping symbols from macOS binary (-u -r)..."
        )
    else()
        if(NOT MSVC)
            # Linux / MinGW GNU strip-all
            add_custom_command(TARGET ${GAME_BIN} POST_BUILD
                COMMAND ${CMAKE_STRIP} -s $<TARGET_FILE:${GAME_BIN}>
                COMMENT "Stripping all symbols from binary (-s)..."
            )
        endif()

        find_program(UPX_EXECUTABLE upx)
        if(UPX_EXECUTABLE)
            add_custom_command(TARGET ${GAME_BIN} POST_BUILD
                COMMAND ${UPX_EXECUTABLE} --ultra-brute $<TARGET_FILE:${GAME_BIN}>
                COMMENT "Compressing Release binary with UPX (--ultra-brute)..."
            )
        endif()
    endif()
endif()

