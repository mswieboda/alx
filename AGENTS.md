# AGENTS.md

## Project Overview
*Aetherlux* (`alx`) is a GBA-aesthetic 2D top-down action-adventure and micro-automation survival crafting game built in C++ with software rendering (`minifb`). Players explore room grids, gather Cursed Alloy, and build mana-conduit infrastructure (Pipes, Refiners, LightSpires) to refine dark twilight mana into stable light energy.

---

## Build & Compilation Instructions

- **Use `make build` to compile**: Always use `make build` when compiling to verify syntax, type safety, and build success without launching the GUI window.
- **Avoid plain `make` / `make run`**: Plain `make` executes the default `all` target (`build run`), which launches the interactive game window. Only run `make` or `make run` when explicitly needing to inspect runtime startup or scene initialization logs.

---

## Planning & Documentation

- Project roadmap and detailed phase breakdown can be found in [PLAN.md](file:///Users/matt/code/cpp/alx/PLAN.md).
- **Note for Agents**: Access `PLAN.md` **sparingly** only when needed for phase context alignment.
