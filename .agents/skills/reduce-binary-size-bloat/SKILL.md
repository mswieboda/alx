---
name: reduce-binary-size-bloat
description: >-
  Audits source code (src/**/*) and git commits for executable binary size bloat, applying modern C++20 size reduction patterns (dead-code elimination, constexpr branch pruning, string literal reduction, inlining hygiene, and template bloat minimization) under strict floppy-disk size constraints.
---

# Reduce Binary Size Bloat Skill (`/reduce-binary-size-bloat`)

This skill audits C++20 source code within [`src/`](file:///Users/matt/code/cpp/alx/src) and commit histories for binary executable size bloat. It provides actionable guidance, compiler heuristics, and targeted refactoring patterns to minimize the final stripped Release binary across all target platforms (macOS Clang, Linux GCC, Windows MSVC) under strict contest floppy-disk size constraints (1,474,560 bytes / 1.44 MB).

---

## 1. Core Engineering Principles & Constraints

1. **Target Scope**:
   - Apply edits **strictly within [`src/`](file:///Users/matt/code/cpp/alx/src)** (`.cpp`, `.h`, `.hpp`).
   - **Never edit files in `lib/` or `deps/`**.
   - Do not rely on platform-specific non-portable hacks or modifying build tools unless explicitly instructed.

2. **Game Jam Size Budget**:
   - **Exact Limit (Bytes)**: 1,474,560 bytes
   - **Limit in KB**: 1,440 KB
   - **Target Safety Margin**: Keep binary as tiny and tight as possible to leave headroom for audio modules, art tiles, and gameplay systems.

3. **High-Engineering Invariants**:
   - Never compromise runtime safety, type safety, or architectural cleanliness for micro-optimizations.
   - Maintain the **Zero Magic Numbers** rule: all extracted constants, offsets, and thresholds must remain named `constexpr` / `inline constexpr` constants in appropriate headers/namespaces.
   - Strictly follow **Header (`.h`) vs. Source (`.cpp`) Separation**.

---

## 2. The 6 Core C++20 Binary Size Reduction Pillars

When auditing or refactoring code for size reduction, apply these 6 core pillars:

### `[DCEG]`: Compile-Time Dead Code Elimination & Macro Gating
- **Problem**: Runtime `if` checks (e.g. `if (Debug::is_enabled)`) require the compiler to emit machine instructions for both branches into the `.text` segment and retain all referenced data in `.rodata`.
- **C++20 Pattern**:
  - Replace runtime `if` conditions with `if constexpr` branches backed by compile-time constants (e.g. `ALX_ENABLE_DEBUG`, `ALX_ENABLE_TELEMETRY`, `ALX_ENABLE_HEADLESS`).
  - When `if constexpr` evaluates to `false`, the C++20 compiler discards the untaken branch during semantic analysis—generating zero machine instructions and omitting unreferenced data/symbols from the final executable.
  - For debug-only subsystems (e.g. `TelemetryDumper`, diagnostic logging), provide zero-cost empty inline stubs in headers when release macros are disabled:
    ```cpp
    #if ALX_ENABLE_TELEMETRY
    void record_metric(std::string_view name, float value);
    #else
    inline void record_metric(std::string_view, float) noexcept {}
    #endif // ALX_ENABLE_TELEMETRY
    ```

### `[SROD]`: Read-Only Data (`.rodata` / `__cstring`) & String Literal Pruning
- **Problem**: String literals (such as format strings, prompt descriptions, log messages, and error tags) are stored directly in the binary's read-only data segment. Repetitive strings, long inline error messages, and temporary string allocations bloat both `.rodata` and `.text`.
- **C++20 Pattern**:
  - Pass string parameters by `std::string_view` to avoid synthesizing dynamic string object allocations (`std::string`) at call sites.
  - Deduplicate repeated strings by sharing `constexpr std::string_view` constants across translation units.
  - Compress repetitive UI/toast prompt text into tokenized templates (e.g. `"{ATTACK}"`, `"{PAN}"`) resolved through small runtime lookup tables rather than embedding full unique strings for every input mode.
  - Gate verbose log and telemetry string literals behind `#if ALX_ENABLE_DEBUG` or `if constexpr` so their string literals are completely omitted from the Release binary.

### `[ICCC]`: Inlining Hygiene & Translation Unit (TU) Header Discipline
- **Problem**: Placing non-trivial multi-statement function bodies inside header files (`.h` / `.hpp`) with `inline` causes every `.cpp` translation unit that includes the header to emit its own copy of the machine code. Linkers with Link-Time Optimization (LTO) attempt to deduplicate them, but complex control flow and distinct TU contexts frequently defeat full dead-stripping.
- **C++20 Pattern**:
  - Keep headers purely declarative: limit header functions strictly to trivial 1-line accessors/mutators, templates, or `constexpr` calculations.
  - Move all multi-branch, loop, or state-mutating method bodies into corresponding `.cpp` source files.
  - For rarely executed cold paths (e.g. error recovery routines, cold scene resets, verbose initialization), mark functions or keep them out of hot inline paths to prevent the compiler from aggressively inlining them into callers.

### `[TIBP]`: Template Instantiation & Monomorphization Pruning
- **Problem**: Each instantiation of a template function or class with a different type generates a distinct copy of compiled code in the `.text` segment. Heavy standard library utilities (like `std::format`, nested `std::variant` visitors, or custom container algorithms) can introduce significant code bloat across translation units.
- **C++20 Pattern**:
  - Prefer non-templated core functions operating on type-erased views (`std::span<const T>`, `std::string_view`) rather than templating on container types.
  - Avoid creating multiple template specializations where a single runtime parameter or small enum branch in a non-templated function suffices.

### `[VTRE]`: Polymorphic & Virtual Dispatch Overhead Elimination
- **Problem**: Classes with `virtual` methods generate virtual method tables (vtables), type descriptors for Run-Time Type Information (RTTI), and indirect call trampolines in the binary.
- **C++20 Pattern**:
  - Favor flat Data-Oriented Design (DOD) structs and enum-based dispatch over deep polymorphic class hierarchies.
  - Eliminate unnecessary virtual destructors and interfaces on internal concrete systems where polymorphism is not strictly required.

### `[LTBT]`: Compact Lookup Tables & Branch Optimization
- **Problem**: Massive cascading `if`/`else` chains or sparse `switch` statements generate extensive branch jump trees and instruction overhead in `.text`. Conversely, oversized uncompressed static arrays in headers bloat `.rodata`.
- **C++20 Pattern**:
  - Replace large repetitive switch/branch statements with small, dense `constexpr std::array` lookup tables indexed by strongly-typed enums (`enum class`).
  - Calculate deterministic data algorithmically at compile time using `constexpr` functions rather than embedding oversized pre-computed static lookup tables into binary data.

---

## 3. Audit & Refactoring Workflow

Follow this step-by-step workflow when auditing binary size:

### Step 1: Baseline Measurement
Always measure and record the baseline Release binary size before making changes:
```bash
task build-release && task size-release
```
Record the exact byte size reported by `ls -lo` or `ls -lah`.

### Step 2: Source Code & Diff Inspection
1. Use `git diff` or inspect target files in [`src/`](file:///Users/matt/code/cpp/alx/src) to locate recent additions.
2. Search for common bloat vectors:
   - Non-constexpr debug checks (`Debug::`, `Log::`).
   - String literals embedded in hot paths or un-gated systems.
   - Non-trivial method bodies implemented directly inside `.h` headers.
   - Redundant template instantiations or temporary object allocations.
   - Unused legacy helper methods or dead structs retained after refactoring.

### Step 3: Precision Source Edits
1. Always re-read target files using `view_file` before editing.
2. Implement C++20 size reductions using `replace_file_content`.
3. Adhere to code standards: zero magic numbers, proper header/source separation, no trailing whitespace, single trailing newline.

### Step 4: Verification & Delta Evaluation
1. Run `task build` to ensure Debug builds compile cleanly without warnings or errors.
2. Run `task build-release && task size-release` to measure the new stripped Release binary size.
3. Compare the before-and-after byte count and summarize the exact delta for the user.

---

## 4. Verification Checklist for Agents

- [ ] All debug, telemetry, and logging paths are gated via `if constexpr` or compile-time preprocessor macros.
- [ ] No non-trivial function bodies reside in `.h` headers (declarations in `.h`, implementations in `.cpp`).
- [ ] Read-only strings use `std::string_view` and are deduplicated or tokenized.
- [ ] Code strictly follows C++20 standards, zero magic numbers, and single level of abstraction (SLAP).
- [ ] Both `task build` and `task build-release` compile cleanly with zero errors.
- [ ] Final binary size remains well within the 1,474,560-byte floppy-disk constraint.
