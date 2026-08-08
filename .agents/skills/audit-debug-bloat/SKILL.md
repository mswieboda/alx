---
name: audit-debug-bloat
description: Audits modified files, specific directories, or game concepts for un-gated debug code, telemetry updates, and runtime CLI flags, automatically applying zero-overhead Release macro guards and if constexpr optimizations.
---

# Audit Debug Bloat Skill (`/audit-debug-bloat`)

This command audits source files (`.cpp`/`.h`) for debug rendering, telemetry metrics, headless logic, and CLI flags that leak into the Release binary. It refactors non-constexpr debug checks into zero-overhead preprocessor guards (`#if ALX_ENABLE_*`) and `if constexpr` branches.

---

## 1. Audit Objective

When `/audit-debug-bloat` is invoked:
1. Inspect target files, staged changes (`git diff --staged`), or modified files (`git diff`).
2. Identify any un-gated telemetry timers, runtime CLI flags, or standard `if` statements checking `Debug::` flags.
3. Apply zero-cost release stubs and compile-time guards.
4. Verify release compilation and binary size via `task build-release` and `task size-release`.

---

## 2. Zero-Overhead Release Rules Checklist

1. **Macro-Gated Debug Constants (`Debug.h`):**
   - Ensure all `Debug::` constants evaluate to `false` when `ALX_ENABLE_DEBUG` is `0`.
2. **Compile-Time Branching (`if constexpr`):**
   - Ensure call sites checking `Debug::` flags use `if constexpr (Debug::...)` instead of runtime `if (...)`.
3. **No-Op Telemetry Stubs (`TelemetryDumper.h`):**
   - Ensure telemetry, reporting, and file dumper methods evaluate to zero-cost inline stubs when `ALX_ENABLE_TELEMETRY` is `0`.
4. **Gated CLI Arguments (`main.cpp`):**
   - Ensure test/debug CLI argument handling (`--headless`, `--report`) is enclosed within `if constexpr (ALX_ENABLE_HEADLESS)`.
5. **Clean Hot-Path Loops (`MainScene.cpp`):**
   - Ensure frame update loops do not increment telemetry timers or calculate debug metrics when `ALX_ENABLE_TELEMETRY` is `0`.
6. **No-Op Console Logging (`Log.h` / `Log.cpp`):**
   - Ensure all `Log::` calls evaluate to inline empty functions in Release mode (`ALX_ENABLE_DEBUG` is `0`).

---

## 3. Audit & Refactoring Execution Workflow

### Step 1: Scan for Debug & Telemetry Patterns
Grep search target files for:
- `Debug::` references lacking `if constexpr`
- Telemetry function calls or frame timers (`m_telemetry_dump_timer`)
- Un-gated CLI argument checks (`has_cli_flag`)
- Un-gated `Log::` invocations carrying large string literals

### Step 2: Apply Precision Edits
- Add inline stubs in header files.
- Enclose source implementations in `#if ALX_ENABLE_*` preprocessor blocks.
- Upgrade runtime `if` conditions to `if constexpr`.

### Step 3: Verify Release Binary Integrity
- Run `task build` to confirm Debug build compiles cleanly.
- Run `task build-release` to confirm Release build compiles cleanly.
- Run `task size-release` to verify binary size remains compact and under contest limits.
