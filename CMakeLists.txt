cmake_minimum_required(VERSION 3.25...4.3)

# CMake 3.25+ / 4.x Global Build Optimizations
set(CMAKE_OPTIMIZE_DEPENDENCIES ON)
if(POLICY CMP0155)
    # Disable C++20 module scanning when compiling standard header includes
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
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

# ----------------------------------------------------
# Build Profiles Configuration (Debug vs Release)
# ----------------------------------------------------

# Target compile definitions (Debug vs Release)
target_compile_definitions(${GAME_BIN} PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

if(MSVC)
    # MSVC Runtime Library & Debug Info abstractions (CMake 3.15+ / 3.25+)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:ProgramDatabase>$<$<CONFIG:Release>:Embedded>")

    # MSVC Compiler Options
    target_compile_options(${GAME_BIN} PRIVATE
        $<$<CONFIG:Debug>:/Od /Zi /RTC1>
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

