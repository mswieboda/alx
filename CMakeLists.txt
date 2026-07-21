cmake_minimum_required(VERSION 3.15)

# Initialize project name dynamically from our Makefile argument
if(NOT GAME_BIN)
    set(GAME_BIN "mini_game")
endif()

project(${GAME_BIN} C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------------------------------
# 🔧 Build Profiles Configuration (Debug vs Release)
# ----------------------------------------------------

# 🗜️ RELEASE FLAGS: Maximize size minimization (-Os), drop unreferenced blocks
set(CMAKE_CXX_FLAGS_RELEASE "-Os -ffunction-sections -fdata-sections -DNDEBUG")

# 🏎️ DEBUG FLAGS: Zero optimization (-O0) for fastest builds, keep symbols (-g)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG")

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

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(X11 REQUIRED)
    target_link_libraries(${GAME_BIN} PRIVATE X11::X11 pthread GL)
endif()

# Only strip and pack if we are building in Release mode
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Strip debug symbols and local symbols (saves significant size on macOS)
    if(APPLE)
        # macOS Mach-O stripping
        add_custom_command(TARGET ${GAME_BIN} POST_BUILD
            COMMAND ${CMAKE_STRIP} -u -r $<TARGET_FILE:${GAME_BIN}>
            COMMENT "Stripping symbols from macOS binary (-u -r)..."
        )
    else()
        # Linux / Windows GNU strip-all
        add_custom_command(TARGET ${GAME_BIN} POST_BUILD
            COMMAND ${CMAKE_STRIP} -s $<TARGET_FILE:${GAME_BIN}>
            COMMENT "Stripping all symbols from binary (-s)..."
        )
    endif()

    # UPX compression (Disabled on macOS due to ARM64 SIP/dyld signature restrictions)
    if(NOT APPLE)
        find_program(UPX_EXECUTABLE upx)
        if(UPX_EXECUTABLE)
            add_custom_command(TARGET ${GAME_BIN} POST_BUILD
                COMMAND ${UPX_EXECUTABLE} --best $<TARGET_FILE:${GAME_BIN}>
                COMMENT "Packing binary with UPX..."
            )
        else()
            message(STATUS "UPX not found. Skipping executable compression.")
        endif()
    else()
        message(STATUS "Skipping UPX on macOS to prevent SIP/arm64 SIGKILL issues.")
    endif()
endif()
