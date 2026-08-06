# [EP-TFAD]: Taskfile Architecture & Dependency Diagnosis Plan

## Overview
This document outlines the diagnostic and optimization plan for resolving dependency tracking issues, execution order quirks, and build-trust deficiencies in `Taskfile.yml`.

---

## [PH-TFAD-1]: Root Cause Hypotheses & Diagnosis

### [TFAD-01]: Missing Task Dependencies on `run`
- **Issue**: Executing `task clean run` fails or runs stale binaries because `run` has no dependency on `build`.
- **Details**: `run` executes `./build/Debug/alx` directly. Calling `clean` removes `build/Debug`, so running `clean` followed by `run` leaves `run` attempting to execute a deleted binary.
- **Fix Hypothesis**: Make `run` depend on `build` (`deps: [build]`), or update standard workflows so `run` always ensures binary freshness.

### [TFAD-02]: Redundant/Conflicting Up-to-Date Checking (`sources` / `generates` vs Ninja)
- **Issue**: `task build` uses go-task `sources` and `generates` checksums to skip tasks before CMake/Ninja runs.
- **Details**: Ninja already maintains a complete C++ dependency graph (including header inclusions and compiler flags). go-task's naive file checksumming can falsely report that `build` is up-to-date when internal header links or CMake settings change, bypassing Ninja recompilation.
- **Fix Hypothesis**: Remove or refine coarse `sources`/`generates` globs on C++ build tasks so Ninja handles incremental compilation deterministically.

### [TFAD-03]: Asset Pipeline Parallelism & Incremental Header Generation
- **Issue**: `assets-parallel` runs `fonts`, `images`, `music` concurrently.
- **Details**: If asset headers (`Fonts.h`, `Images.h`, `Music.h`) are deleted or dirty, parallel execution of `pack-assets-bin` might race or produce out-of-sync mtimes relative to C++ compilation.
- **Fix Hypothesis**: Verify atomic generation and explicit task ordering for asset headers.

### [TFAD-04]: Cache / Checksum Disconnect on `clean`
- **Issue**: `clean` removes build artifacts (`rm -rf build/Debug`), but does not invalidate go-task's internal checksum database (`.task/checksum`).
- **Details**: When `build` is run after partial clean operations, go-task may operate on stale checksum cache states.
- **Fix Hypothesis**: Ensure `clean` task purges `.task/checksum` or that task configurations properly detect missing targets.

---

## [PH-TFAD-2]: Verification & Testing Strategy

### [TFAD-05]: Test Matrix Execution in Worktree
- Test Scenario 1: `task clean run` (Verify binary builds prior to launch).
- Test Scenario 2: Edit C++ `.cpp` file & test header -> `task` (Verify Ninja triggers rebuild without go-task skipping).
- Test Scenario 3: Edit asset file -> `task` (Verify asset packer rebuilds headers and triggers re-compilation).
- Test Scenario 4: Delete single object/header -> `task build` (Verify auto-recovery).

---

## [PH-TFAD-3]: Proposed Refactored `Taskfile.yml` Architecture

### [TFAD-06]: Simplified & Deterministic Pipeline Structure
- `run` Task: `deps: [build]` to guarantee executable existence.
- `build` Task: Hand off incremental checks to Ninja/CMake for C++ source trees; use `sources` strictly for asset packing inputs where Crystal generator scripts are involved.
- `clean` Task: Include `.task` cache invalidation so go-task checksums reset on clean operations.
