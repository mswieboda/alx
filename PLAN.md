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

We will lean heavily into that action-adventure GBA vibe where exploring rooms, taking down shadow-wraiths, and harvesting resources feels active rather than just sitting back and solving a spreadsheet puzzle.

And using a debug cheat (like pressing `5` to instantly spawn "Cursed Alloy") right now is a genius game development shortcut. It lets you skip the tedious resource grinding during early greybox testing so you can immediately focus on getting your player movement, grid placement, and pipe routing working cleanly.

Here is a pragmatic, **phased roadmap** to take *Aetherlux* from your current debug-key state all the way to the full Loop 2 survival-crafting loop without getting overwhelmed before September 4th:

##### Phase 3.0: Tile interactions

* (done) we can detail it out later, but this is done
* Seep (dark mana) -> Pipe (dark mana) -> Refiner (powered on, dark mana consumed) -> Pipe (light mana) -> Light Spire (consumes light mana)
* Light mana has a time-to-life expiration and disappears if not used in a Light Spire

##### Phase 3.1: The "Debug Crutch" Baseline (Current Step)

* **Objective:** Keep building mechanics frictionless while you code core movement and grid placement.
* **Implementation:** * Add a simple integer tracker in your scene or player class: `int m_cursed_alloy = 0;`.
* Bind a key (like `5`) in your input loop: `if (keys[GLFW_KEY_5]) { m_cursed_alloy += 10; }` (or whatever your keyboard polling structure uses).
* Make placing a pipe decrement `m_cursed_alloy`. If it hits 0, placement fails.


* **Why it works:** You can instantly test your pipe-placement logic, pathfinding, and layout validation without needing enemies or drop tables programmed yet.

#### Phase 2.2: Automated / Random Resource Spawning (The Crystal Node Cracks)

* **Objective:** Introduce randomized "Cursed Alloy" nodes spawning organically across the room (e.g., near outer stone walls or dark corners) without requiring enemies yet.
* **Logic / Implementation:**
* In your `alx::Grid` or scene update loop, add a timer (e.g., every 5 seconds). When it triggers, find a random open floor tile near the room's edges and mark its state or spawn an item structure (`alx::ItemDrop` entity) with a pulsing dark-purple color indicator.
* Maybe even there are 2 states, first it is a "crack" in the floor, then when you press E it turns into the ItemDrop that pressing E collects as Cursed Alloy (our resource to build things)
* Keep a simple counter or container of active drops on the grid, and put in a super basic HUD at the top.


---

#### **Phase 4: Combat, Spawners & Content Rooms**

* Spawning shadow-pests from dark room corners that attack your pipes and player
* Quick-swipe mystic tool / blast attack / defense mechanic

##### Phase 4.1: Basic Enemy Spawns & Hitboxes (The Threat)

* **Objective:** Introduce simple shadow-pests that move or spawn inside the room to harass your grid.
* **Implementation:** * Define a basic `alx::Enemy` struct or class (similar to your player box).
* Give them simple AI (e.g., drift slowly toward the player or drift toward the nearest active pipe tile to sabotage it).
* Implement simple AABB collision between your Mystic Adept's attack tool (e.g., a quick melee swing when pressing Spacebar) and the enemy boxes.



##### Phase 4.2: The Loot Drop & Harvesting Link (The Economy Transition)

* **Objective:** Remove the need for the debug `5` key by replacing it with actual gameplay rewards.
* **Implementation:** * When an enemy's health hits zero via your weapon swing, destroy the enemy object and spawn a small visual token on that tile (e.g., a pulsing dark-purple pixel block: `alx::ItemDrop`).
* When your Mystic Adept walks over the token, it automatically collects into your `m_cursed_alloy` inventory.
* *Milestone:* You can now build pipes entirely using resources harvested from defeating enemies!



##### Phase 4.3: Infrastructure Sabotage (Raising the Stakes)

* **Objective:** Give enemies a reason to attack your infrastructure so the automation loop actually matters.
* **Implementation:** * Allow shadow-pests to "bite" or degrade pipe tiles if they occupy the same grid index. A damaged pipe stops flowing twilight fluid.
* This forces the player into a constant tactical loop: *Do I leave my base to hunt more alloy, or stay back and defend the active conduit lines?*

#### **Phase 5: Level Building**

* Constructing the 3 core vertical levels:
1. *The Damp Cellar Crypt* (Tutorial & initial pipe routing)
2. *The Ruined Gothic Hall* (Wider automation puzzle)
3. *The Open-Air Spire Rooftop* (Final gateway beacon activation)


#### **Phase 6: Polish, Audio & Final Size Check**

* Adding UI text crawls and the title screen ("*Aetherlux*")
* Verifying the final binary build stays tightly under the **1.44Mb Floppy Disk** limit.
