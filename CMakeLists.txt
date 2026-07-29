cmake_minimum_required(VERSION 3.15)

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
# Build Profiles Configuration (Debug vs Release)
# ----------------------------------------------------

if(MSVC)
    # MSVC FLAGS (Windows)

    # Debug: /Od (no opt), /Zi (debug info), /RTC1 (runtime checks)
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /RTC1 /DDEBUG")

    # MSVC RELEASE FLAGS (Max Size Reduction)
    # /O1   = Minimize size
    # /Os   = Favor small code
    # /Ob1  = Only expand functions marked inline (prevents code bloat)
    # /GF   = String pooling (merges duplicate literal strings)
    # /Gy   = Function-level linking (enables dropping unused functions)
    # /Gw   = Global data optimization (enables dropping unused global data)
    # /GL   = Whole program optimization (LTCG compiler flag)
    set(CMAKE_CXX_FLAGS_RELEASE "/O1 /Os /Ob1 /GF /Gy /Gw /GL /DNDEBUG")

    # MSVC LINKER FLAGS (Max Size Reduction)
    # /OPT:REF = Eliminate unreferenced functions/data
    # /OPT:ICF = Merge identical function bytes (Identical COMDAT Folding)
    # /LTCG    = Link-Time Code Generation
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF /LTCG")
else()
    # GCC / CLANG FLAGS (macOS & Linux)
    # DEBUG FLAGS: Zero optimization (-O0) for fastest builds, keep symbols (-g)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG")

    # RELEASE FLAGS: Maximize size minimization (-Os), drop unreferenced blocks
    set(CMAKE_CXX_FLAGS_RELEASE "-Os -ffunction-sections -fdata-sections -DNDEBUG")
endif()

if(APPLE)
    # Strip symbols and dead code *only* on a Release build target
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-dead_strip")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # Strip symbols and garbage-collect unreferenced sections on Linux Release
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -s")
endif()

# ----------------------------------------------------

include_directories(deps)
include_directories(src)
include_directories(deps/miniaudio)
include_directories(deps/pocketmod)

set(MINIFB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MINIFB_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(deps/minifb)

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE C_SOURCES CONFIGURE_DEPENDS "src/*.c")

add_executable(
    ${GAME_BIN}
    ${CPP_SOURCES}
    ${C_SOURCES}
)

target_link_libraries(${GAME_BIN} PRIVATE minifb)

# Platform-specific system dependencies
if(WIN32)
    # Required Windows system libraries for MiniFB & MiniAudio
    target_link_libraries(${GAME_BIN} PRIVATE user32 gdi32 shell32 winmm dwmapi opengl32)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(X11 REQUIRED)
    target_link_libraries(${GAME_BIN} PRIVATE X11::X11 pthread GL)
endif()

# Only strip and pack if we are building in Release mode
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(APPLE)
        # macOS Mach-O stripping
        add_custom_command(TARGET ${GAME_BIN} POST_BUILD
            COMMAND ${CMAKE_STRIP} -u -r $<TARGET_FILE:${GAME_BIN}>
            COMMENT "Stripping symbols from macOS binary (-u -r)..."
        )
    elseif(NOT MSVC)
        # Linux / MinGW GNU strip-all
        add_custom_command(TARGET ${GAME_BIN} POST_BUILD
            COMMAND ${CMAKE_STRIP} -s $<TARGET_FILE:${GAME_BIN}>
            COMMENT "Stripping all symbols from binary (-s)..."
        )
    endif()
endif()
