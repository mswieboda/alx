---
name: targeted-cpp-refactoring
description: Perform targeted, high-engineering C++ code refactoring, code review, modular abstraction, readability cleanup, method decomposition, human-centric naming, and performance optimization on specific files or game concepts (e.g., player movement, rendering, memory efficiency) while maintaining strict focus and steering off-topic requests back to the refactoring scope.
---

# Targeted C++ Refactoring Skill (`targeted-cpp-refactoring`)

This skill provides a rigorous, standardized workflow for AI coding agents to perform high-engineering C++ code refactorings, code reviews, cleanups, abstractions, method decompositions, and performance audits focused on specific files or domain concepts within the codebase.

---

## 1. Trigger Criteria & Activation

Activate this skill when:
- The user asks to refactor, clean up, abstract, review, optimize, or modernize specific C++ files (e.g., `Player.cpp`, `DrawFixtures.cpp`).
- The user requests refactoring or optimization of a specific game concept or subsystem (e.g., "player movement", "non-sprite primitive rendering", "memory allocation in update loop", "twilight pipe grid performance").
- The user invokes a targeted code review or architecture audit.

---

## 2. Context Resolution Protocol

When invoked, immediately determine the target scope using the following precedence:

### Mode A: Specific File(s) Scope
If the user specifies exact file names (e.g. `src/Player.cpp`, `src/Player.h`):
1. Resolve both the source (`.cpp`) and header (`.h`/`.hpp`) files for the requested symbols.
2. Read the files using `view_file` to inspect the exact current code state.

### Mode B: Conceptual / Subsystem Scope
If the user specifies a game concept or feature (e.g., "player movement", "mana pipe fluid simulation", "primitive drawing"):
1. Perform targeted code search (`grep_search`) to discover all files, structs, classes, and methods implementing that concept.
2. Identify the core files housing the logic and state.
3. List the discovered files and confirm the target scope with the user before mutating code.

### Mode C: Ambiguous / Missing Context (Smart Inference Protocol)
If the user invokes the skill without providing specific files or concepts:
1. Automatically inspect current git status or recently edited files.
2. If recent context clearly indicates the target area, state the inferred target scope to the user.
3. If context remains ambiguous, prompt the user with 2-3 specific options to clarify the target scope before proceeding.

---

## 3. Off-Topic Steering Protocol

If the user submits a prompt during an active refactoring session that strays off-topic (e.g., requesting unrelated new features or fixing non-refactoring bugs outside the scope):

1. **Acknowledge & Remind**: Politely remind the user of the active refactoring focus.
2. **Present Choice**: Provide two explicit options:
   - **Option A**: Complete or pause the active refactoring focus first.
   - **Option B**: Overrule the current skill context and switch directly to the new request.
3. **Wait for Clarification**: Do not mutate un-scoped files until the user explicitly confirms Option B or overrules.

---

## 4. General Software Engineering & Clean Architecture Standards

All code modifications under this skill MUST satisfy both codebase-specific rules and universal software engineering best practices:

### A. Method Decomposition & Single Level of Abstraction (SLAP)
- **Composed Method Pattern**: High-level entry methods must read like an outline or table of contents, delegating step details to composed helpers.
- **Single Level of Abstraction (SLAP)**: Do not mix high-level business policy (e.g. `processCombatTurn()`) with low-level details (e.g. raw array index math, bitwise flags, or string formatting).
- **Target Method Metrics**: Aim for functions $\le 25$ lines or $\le 5$ statements. Keep cognitive complexity low.
- **Guard Clauses & Bouncers**: Handle precondition checks and edge cases at the top of functions with early `return`/`continue` statements to eliminate nested `if/else` ladders.

### B. Encapsulation & Helper Placement Rules
- **Anonymous Namespaces in `.cpp` (`namespace { ... }`)**: Place stateless algorithmic helpers and pure functions inside anonymous namespaces in source files to prevent header bloat and isolate translation unit symbols.
- **Private Class Helpers**: Reserve `private` member functions in headers strictly for helpers that mutate or directly read private instance state (`m_members`).
- **Inline Lambdas**: Use inline lambdas for one-off predicates used within a single method (e.g., `std::ranges::find_if`).

### C. Human-Centric Concise Naming (Eliminating AI Verbosity)
- **Human-Centric vs. AI Verbosity**: Code is read 10–20x more often than written. Avoid redundant AI-generated noise words (`Manager`, `Helper`, `Processor`, `DataInfoStruct`, `Instance`). Use concise, semantically dense domain terms (`Store`, `Registry`, `Config`, `Bounds`).
- **The Scope Law**: Identifier length is proportional to scope lifetime:
  - Tight local loops (1–5 lines): Short names (`x`, `y`, `pos`, `ctx`, `tile`).
  - Broad public APIs: Short, punchy action names (`render()`, `update()`, `clear()`, `save()`).
  - Deep internal helpers: Specific, descriptive names (`calculate_pipe_flow_overhang()`).
- **No Scope Redundancy**: Do not repeat class name inside members (e.g. `Room::draw()` instead of `Room::draw_room()`).
- **Grammatical Blueprinting**:
  - Structs/Classes = Nouns (`TileGrid`, `PipeNetwork`).
  - Action Methods = Verbs (`refine_mana()`, `sort()`).
  - Predicate Methods = Interrogatives (`is_active()`, `has_alloy()`, `can_build()`).
  - Always phrase booleans in the positive (`is_enabled` instead of `is_not_disabled`).

### D. Ruby/Crystal-Style Expressive Readability in C++20
- **Predicate Queries**: Replace raw integer checks or getter flags with expressive boolean predicates (`user.is_active()`, `pipe.is_connected()`).
- **C++20 Designated Initializers**: Replace functions taking $>2$ positional arguments or boolean parameter traps with `struct Options` initialized via `.field = value`.
- **C++20 Ranges Pipelines**: Use pipe composition (`list | std::views::filter(...) | std::views::transform(...)`) for declarative collection filtering without temporary vectors.
- **Strongly Typed Quantities & Literals**: Wrap raw numbers in domain types (`HealthPoints`, `Seconds`) and define user-defined literals (`100_hp`, `10_sec`, `75_percent`) to eliminate unit-mixing bugs.

### E. Object Architecture & Law of Demeter (LoD)
- **Law of Demeter**: Avoid train-wreck calls (`a.getB().getC().getD().doSomething()`). Delegate methods or pass exact parameter structs down.
- **SOLID Principles**:
  - **SRP**: Single Responsibility Principle at both class and method scopes.
  - **DIP**: High-level game logic depends on abstract interfaces (`IRenderer`, `IAudioEngine`), not concrete framework libraries.
  - **Composition Over Inheritance**: Prefer flat data structs and system functions over deep inheritance hierarchies.

### F. Zero Magic Numbers & Dynamic Sizing
- **No Raw Literals**: Never hardcode raw numeric literals (`16`, `32`, `8.0f`, pixel offsets) in math, collision, layout, or rendering.
- **Single Source of Truth**: Query existing getters, properties, or shared metadata (e.g., `tile_size`, `room.getTileWidth()`).
- **Dynamic Layout Math**: Calculate layout and entity bounds dynamically from container dimensions. If a constant is missing, introduce a named `constexpr` or static constant in an appropriate header/namespace.

### G. Header (`.h`) vs. Source (`.cpp`) Separation
- **Declarations in Headers, Implementations in Source**: Headers contain ONLY declarations, class definitions, `constexpr`, and templates. Function and method bodies MUST reside in `.cpp` source files.
- **Minimal Header Bloat**: Use forward declarations (e.g., `class Player;`, `struct Room;`) in headers. Keep heavy `#include` directives strictly in source `.cpp` files.
- **Exceptions**: Inline 1-line accessors/mutators, `constexpr` functions, and C++ templates.

### H. Frame Loop Hygiene & Hot-Path Performance
- **Zero Dynamic Allocations in Loops**: Zero heap allocations (`new`, `malloc`, dynamic `vector::push_back` resizes) inside `update()` or `render()` loops. Pre-allocate collections, reuse draw buffers, and avoid temporary objects per frame.

### I. Software Rendering & Framebuffer Hygiene
- **Decoupled Logic**: Separate state updates (`update(float dt)`) from rendering passes (`render()`).
- **Clip Bounds Checking**: Always enforce clip rectangle checks (`x >= 0 && x < width && y >= 0 && y < height`) before writing to raw pixel buffers (`buffer[y * screen_width + x]`) to prevent buffer overflows.

---

## 5. Codename Acronym Output Formatting

All refactoring plans, proposal breakdowns, tasks, and data structure designs generated during a refactoring session MUST strictly follow the **Codename Acronym Format**:

`[ACRONYM]`: Title - brief description/details (including concise C++ struct/function suggestions).

### Acronym Rules:
- **Prefixes**:
  - Epics / Major Refactors: `[EP-SCTE]` (e.g. `[EP-PLMV]` for Player Movement Refactor)
  - Phases: `[PH-SCTE]` (e.g. `[PH-DRAW]` for Drawing System Phase)
  - Sub-Tasks / Data Structures / Items: `[SCTE]` (no prefix, 1-6 uppercase letters, e.g. `[ZMNM]` for Zero Magic Numbers Migration)

---

## 6. Refactoring Execution Workflow

Follow this step-by-step process when carrying out a refactoring task:

### Phase 1: Context Gathering & Code Inspection
- Read all target files using `view_file`.
- Re-read files immediately before making any edit to ensure no stale content is used.

### Phase 2: Refactoring Proposal & Audit
- Output a structured proposal using the `[ACRONYM]` format detailing:
  1. Identified anti-patterns, method length, and standards violations.
  2. Proposed structural, naming, and architectural changes.
  3. Header and source file updates.

### Phase 3: Incremental Implementation
- Make edits using `replace_file_content` or `multi_replace_file_content`.
- Edit header files first (declarations & types), followed by source files (`.cpp`).
- Enforce strict no trailing whitespace and ensure a single trailing newline at the end of every file.

### Phase 4: Build & Syntax Verification
- Always compile the codebase immediately after edits to verify syntax and type safety:
  ```bash
  task build
  ```
- If compilation fails, inspect the error output and resolve root causes immediately without adding symptom patches.

---

## 7. Skill References

For detailed code transformation patterns, Before vs After examples, and design recipes, consult:
- [cpp_refactoring_checklist.md](references/cpp_refactoring_checklist.md)
