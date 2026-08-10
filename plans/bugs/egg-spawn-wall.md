bug:
- i saw an enemy spawn (likely the egg too) outside the boundaries of the map, or likely in a wall. and i saw them spawn inside/on top of fixture buildings (refiner/spire)

- FIRST fix the above bug, likely where the DarkTower is allowed to spawn eggs, make sure they are within the map and don't spawn on things that are "solid" or whatever like walls, fixture buildings, the player, an enemy etc. if it fails with "solid" then simply pick a different random spot again, if it fails like 3 times, just stop and don't spawn the egg. i do definitely see them spawn inside/on top of fixtures

- the map boundary isn't really an issue anymore since the Enemy will move to the inner boundaries of the map when outside, via movement states

- double check for solid tiles like water/empty tiles etc
