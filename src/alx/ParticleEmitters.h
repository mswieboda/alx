#pragma once

namespace alx {

class ParticleSystem;

namespace ParticleEmitters {

void spawn_sword_slash_trail(ParticleSystem& ps, float prev_tip_x, float prev_tip_y, float curr_tip_x, float curr_tip_y, float swing_progress, int count = 16);
void spawn_hit_sparks(ParticleSystem& ps, float x, float y, int count = 25);
void spawn_spire_embers(ParticleSystem& ps, float x, float y, int count = 1);
void spawn_refiner_embers(ParticleSystem& ps, float x, float y, int count = 1);
void spawn_alloy_pickup(ParticleSystem& ps, float x, float y, int count = 15);
void spawn_straight_pipe_mana(ParticleSystem& ps, int tile_x, int tile_y, int dir_x, int dir_y, int count = 2, int tile_size = 16);

} // namespace ParticleEmitters
} // namespace alx
