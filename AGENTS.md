# AGENTS.md

## Project Overview
*Aetherlux* (`alx`) is a GBA-aesthetic 2D top-down action-adventure and micro-automation survival crafting game built in C++ with software rendering (`minifb`) as well as audio (`miniaudio`), mod support (`pocketmod`), and gamepad (`minigamepad` support). Players explore room grids, gather Alloy, and build mana pipe infrastructure (Pipes, Refiners, Spires) to refine dark twilight mana into stable light energy to reduce twilight in the room/level.

---

## Build & Compilation Instructions

- **Use `task build` to compile**: Always use `task build` when compiling to verify syntax, type safety, and build success without launching the GUI window.
- **Avoid plain `task` / `task run`**: Plain `task` executes the default target (`build run`), which launches the interactive game window. Only run `task` or `task run` when explicitly asked to, to inspect runtime startup or scene initialization logs.
- **Deprecated build pipeline**: GNU Make and the `Makefile` have been deprecated for this project. Running it will output in a warning/error and will not do anything.

---

## Planning & Documentation

- Project roadmap and detailed phase breakdown can be found in in several `.plans/PLAN*.md` files for different concepts, as scratchpads for plan phased roadmaps.
- **Note for Agents**: Access these **sparingly** only when needed, or when requested, for phase context alignment.

## Brainstorming & Task Formatting

When collaborating on feature planning, roadmaps, task breakdowns, subtasks, data structures, or architectural designs, strictly format every item using the **Codename Acronym Format**:

`[ACRONYM]`: Title - brief description/details (include concise C++ struct suggestions if applicable, e.g., `DarkTower struct: float x, y; int hp = 8;`).

### Naming & Prefix Rules
- **Acronym**: 1 to 6 uppercase letters derived from title words (e.g., `SCTE` for "Short Concise Title of Epic").
- **Epics**: Prefix `EP-` &rarr; `[EP-SCTE]`
- **Phases**: Prefix `PH-` &rarr; `[PH-SCTE]`
- **Sub-Tasks / Tasks / Data Structures / Bulleted Items**: No prefix &rarr; `[SCTE]`
