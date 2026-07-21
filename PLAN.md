### Phased Plan: *Aetherlux* (`alx`)**

#### **Phase 1: The Greybox & Runtime Core (Next Up 🎯)**

* *Objective:* Get a playable, non-textured logic loop running on screen using primitive shapes before touching actual sprites.
* **1.1 The Tilemap Grid Data Structure (`alx::Grid`):** * Define a simple 2D array or vector representing the room grid (e.g., floor, wall, pipe conduit slot).
* **1.2 Render Loop for Primitive Blocks:** * Write a function to draw filled rectangles directly to your `minifb` pixel buffer (e.g., dark grey blocks for walls, lighter grey for floors).
* **1.3 The Boxy Player Entity (`alx::Player`):** * Render the Mystic Adept as a simple colored box or placeholder shape that responds smoothly to directional input (`WASD` / Arrow Keys) with delta time.
* **1.4 Basic Grid Collision:** * Implement simple AABB (Axis-Aligned Bounding Box) or tile-index collision checks so your boxy player cannot walk straight through walls.

#### **Phase 2: GFX Asset Pipeline & Aseprite Template Integration**

* Defining the 256-color indexed palette (midnight blues, bruised purples, cyan, amber)
* Setting up sprite templates (using Slynyrd’s top-down character proportions)
* Drafting the 16x16 tile sheet (gothic stone floors, walls, and conduit pipe wireframes)
* Writing a lightweight asset parser or indexed pixel array loader to ingest your exported Aseprite graphics into the C++ `alx` namespace.

#### **Phase 3: Pipe-Routing Logic & Micro-Factorio Mechanics**

* Implementing tile interaction (laying down conduit pipe pieces across the floor grid)
* Adding resource states: Twilight Seep generation -> raw purple-black mana flow -> Refiner Prism conversion -> stable light output.

#### **Phase 4: Combat, Spawners & Content Rooms**

* Spawning shadow-pests from dark room corners that attack your pipes and player
* Quick-swipe mystic tool / blast defense mechanic
* Constructing the 3 core vertical levels:
1. *The Damp Cellar Crypt* (Tutorial & initial pipe routing)
2. *The Ruined Gothic Hall* (Wider automation puzzle)
3. *The Open-Air Spire Rooftop* (Final gateway beacon activation)



#### **Phase 5: Polish, Audio & Final Size Check**

* Adding UI text crawls and the title screen ("*Aetherlux*")
* Verifying the final binary build stays tightly under the **1.44Mb Floppy Disk** limit.
