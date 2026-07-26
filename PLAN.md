### Loose Plan Scratchpad: *Aetherlux* (`alx`)**

#### GFX Asset Pipeline & Aseprite Template Integration

* Defining the 256-color indexed palette (midnight blues, bruised purples, cyan, amber)
* Setting up sprite templates (using Slynyrd’s top-down character proportions)
* Drafting the 16x16 tile sheet (gothic stone floors, walls, and conduit pipe wireframes)
* Writing a lightweight asset parser or indexed pixel array loader to ingest your exported Aseprite graphics into the C++ `alx` namespace.

##### Brainstorm and Sprite Templates

- top down character template tutorial https://www.slynyrd.com/blog/2025/3/24/pixelblog-55-top-down-character-animation
- top down character attach animation tutorial https://www.slynyrd.com/blog/2025/5/23/pixelblog-56-top-down-character-attack-animation
- free 2D assests characters/tiles to tweak for templates https://anokolisa.itch.io/free-pixel-art-asset-pack-topdown-tileset-rpg-16x16-sprites

---

##### Phase 4.3: Infrastructure Sabotage (Raising the Stakes)

* **Objective:** Give enemies a reason to attack your infrastructure so the automation loop actually matters.
* **Implementation:** * Allow shadow-pests to "bite" or degrade pipe tiles if they occupy the same grid index. A damaged pipe stops flowing twilight fluid.
* This forces the player into a constant tactical loop: *Do I leave my base to hunt more alloy, or stay back and defend the active pipe mana flow lines?*

---

#### **Phase 5: Level Building**

* Constructing the 3 core vertical levels:
1. *The Damp Cellar Crypt* (Tutorial & initial pipe routing)
2. *The Ruined Gothic Hall* (Wider automation puzzle)
3. *The Open-Air Spire Rooftop* (Final gateway beacon activation)

---

#### **Phase 6: Polish, Audio & Final Size Check**

* Adding UI text crawls and the title screen ("*Aetherlux*")
* Verifying the final binary build stays tightly under the **1.44Mb Floppy Disk** limit.
