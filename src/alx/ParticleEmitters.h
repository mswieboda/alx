#pragma once

namespace alx {

class ParticleSystem;

namespace ParticleEmitters {

void spawn_sword_slash_trail(ParticleSystem& ps, float prev_tip_x, float prev_tip_y, float curr_tip_x, float curr_tip_y, float swing_progress, int count = 16);
void spawn_hit_sparks(ParticleSystem& ps, float x, float y, int count = 10);
void spawn_spire_embers(ParticleSystem& ps, float x, float y, int count = 1);
void spawn_refiner_embers(ParticleSystem& ps, float x, float y, int count = 1);
void spawn_alloy_pickup(ParticleSystem& ps, float x, float y, int count = 8);

} // namespace ParticleEmitters
} // namespace alx
