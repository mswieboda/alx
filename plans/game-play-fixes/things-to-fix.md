- player needs to be able to move (and actually face, instead of strafe) in all directions while building. but only pressing A or holding A locks the direction down

- player needs: Player Dash / Dodge Roll (maybe the GBA-style "B" button or like a new usage of controller X/Y which isn't technically GBA-style but usually on all modern game controllers, so it would be fine to add in Input.h/Action.h and cpp etc)

- when the direction is locked down in building mode, it can only be in N/E/S/W, never diagonal, if the player starts diagonal, or moves diagonal the tile needs to be calculated (consistently) based on rules like, if they are locked facing N initially and move diagonally, then it should stay N, if they instantly swap to SW or whatever we need to decide (via planning here and brainstorming) if it should flip to S or W, i would argue it should be S so it's more "opposite" of their previous direction. but we need to consider these deeply and better options if there are good ones. and consider whether this is a good game mechanic and why not

- indicator near twilight percentage that shows the rolling 15s AVG rate, but as a white (a lot), light yellow (slight) arrow if decreasing twilight, and light purple (light) and dark purple (a lot) if twilight is decreasing
- alloy pickups need to give like 3 alloy or 5 allow instead of just 1, or give a rare randomness for a "big" one like 5 alloy big brick (and all others are 2)

- we need to create an AI skill for the repo, that is "/is-good-game-mechanic" that asks if whatever the user is asking (new feature), or an existing feature (via telling, or specific files) is a good game mechanic and why or why not, and compare to common 2D games like factorio, mindustry, stardew valley, harvest moon, etc. to help us determine the best option for game design and game mechanics

- twilight levels are still not working well, we need to tweak them so they are better so the player gets rewarded for building and fighting. but not overwhelmed like it's impossible. this will be a hard balance to strike

- player spark projectile needs to be bigger in size so it hits enemies or other things easier
