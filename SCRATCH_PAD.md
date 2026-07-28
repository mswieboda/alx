notes:

- player should not move while attacking (and keep the exact last facing direction before attacking, in case WASD was released etc)
- threat indicators cache needs to not be the actual enemies tracked, but the exact last positions of all the enemies when scanned, so is just a pure cached list of x,y positions to display on the screen, or the enemy positions, etc whichever is better to do in `update()` vs in `draw()`
- TBD

asdfsdf