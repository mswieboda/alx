---
name: check-best-practices-alx
description: >-
  Audits git staged changes (`git diff --staged`) and modified code against the project's engineering best practices in AGENTS.md and the refactor-alx skill, executing refactoring fixes while enforcing strict topic focus.
---

# Check Best Practices Skill (`/check-best-practices-alx`)

This command audits staged (`git diff --staged`) and modified codebase changes against the project standards defined in [`AGENTS.md`](file:///Users/matt/code/cpp/alx/AGENTS.md) and the [`refactor-alx`](file:///Users/matt/code/cpp/alx/.agents/skills/refactor-alx/SKILL.md) skill. It ensures all changes comply with high-engineering quality, C++20 best practices, and clean architecture before code is committed.

---

## 1. Goal & Pre-Prompt Instructions

When this command (`/check-best-practices-alx`) is invoked, execute the following pre-prompt instruction:

> **Audit Objective**: Inspect all staged files (`git diff --staged`), as well as any active modified files (`git diff`), to verify complete compliance with our engineering best practices outlined in `AGENTS.md` and the `refactor-alx` skill. Identify any violations, perform targeted refactoring to fix root causes, and verify build integrity via `task build`.

---

## 2. Audit Workflow & Execution Protocol

Follow these sequential steps during a `/check-best-practices-alx` audit:

### Step 1: Inspect Staged & Unstaged Diff
- Run `git status` and `git diff --staged` to discover all modified and staged files.
- If no files are currently staged, inspect modified files using `git diff` and inform the user.
- Identify every `.h`/`.hpp` header and `.cpp` source file altered in the diff.

### Step 2: Read Full Target File Context
- Use `view_file` to inspect the full contents of modified source and header files (always re-reading files immediately before making any edit).

### Step 3: Best-Practices Compliance Audit
Check the staged and modified code against the following explicit standards:

1. **Zero Magic Numbers**:
   - Ensure no hardcoded raw numeric literals (e.g. `16`, `32`, `8.0f`, static layout/render offsets) exist in logic, math, or rendering.
   - Extract constants to shared metadata getters, properties, or `constexpr` values.
2. **Header (`.h`) vs. Source (`.cpp`) Separation**:
   - Header files must contain ONLY declarations, types, `constexpr`, and templates.
   - Move non-inline function and method definitions out of headers and into `.cpp` source files.
3. **Modern C++20 & Safe Types**:
   - Verify usage of explicit types (`uint32_t`, `int32_t`), `constexpr`, `enum class`, and read-only views (`std::string_view`, `std::span`).
   - Enforce `const` correctness on all getters and inspectors.
   - Use in-class member initializers (`int m_val{0};`).
4. **Frame Loop & Hot-Path Performance**:
   - Zero dynamic memory allocations (`new`, `malloc`, dynamic `vector::push_back` resizes) inside `update()` or `render()` loops.
5. **Software Rendering & Framebuffer Hygiene**:
   - Decouple update logic from draw logic.
   - Enforce explicit clip rectangle bounds checks (`x >= 0 && x < width && y >= 0 && y < height`) prior to raw framebuffer index writes (`y * screen_width + x`).
6. **Single Level of Abstraction (SLAP) & Method Decomposition**:
   - Keep entry methods clean and high-level ($\le 25$ lines). Decompose low-level logic into composed helper functions.
   - Place stateless internal helpers inside anonymous namespaces (`namespace { ... }`) in `.cpp` files.
7. **Human-Centric Concise Naming**:
   - Eliminate redundant AI noise words (`Manager`, `Helper`, `Processor`, `DataInfoStruct`).
8. **Codename Acronym Formatting**:
   - Format any task breakdowns or refactoring proposals using `[ACRONYM]` tags (e.g. `[EP-SCTE]`, `[PH-SCTE]`, `[SCTE]`).

### Step 4: Refactor & Fix Violations
- Apply precision edits using `replace_file_content` or `multi_replace_file_content`.
- Modify header files first, then corresponding source files.
- Ensure no trailing whitespace and exactly one trailing newline per file.

### Step 5: Compilation & Syntax Verification
- Run `task build` to verify syntax, type safety, and clean compilation.
- Resolve any build errors immediately at the root cause.

---

## 3. Additional Prompts & Focus Incorporation

Any additional instructions or prompt parameters provided by the user alongside `/check-best-practices-alx` (e.g., `/check-best-practices-alx focus on collision logic in Player.cpp`) are **valid and active**.

- Incorporate the additional prompt to narrow or expand the audit focus as requested.
- Maintain all core best-practices checks while prioritizing the user's specific area of focus.

---

## 4. Off-Topic Steering Protocol

If the user submits follow-up prompts or requests during a `/check-best-practices-alx` session that stray off-topic from checking or refactoring the staged/modified code (e.g. asking to implement unrelated features or fix un-scoped bugs):

1. **Remind the User First**:
   Politely inform the user that the primary goal of the active session is auditing and refactoring staged changes against `AGENTS.md` and `refactor-alx` standards.

2. **Present Clear Options**:
   Provide two explicit choices:
   - **Option A**: Complete or pause the best-practices audit/refactoring task first.
   - **Option B**: Overrule the active `/check-best-practices-alx` goal and switch focus to the new request.

3. **Wait for Clarification**:
   Do not perform off-topic file modifications until the user explicitly confirms Option B or overrules the current goal.
