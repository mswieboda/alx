#pragma once

namespace alx {

class ParticleSystem;

namespace ParticleEmitters {

void spawn_sword_slash_trail(ParticleSystem& ps, float arc_cx, float arc_cy, float arc_radius);
void spawn_hit_sparks(ParticleSystem& ps, float x, float y, int count = 10);
void spawn_spire_embers(ParticleSystem& ps, float x, float y, int count = 1);
void spawn_refiner_embers(ParticleSystem& ps, float x, float y, int count = 1);
void spawn_alloy_pickup(ParticleSystem& ps, float x, float y, int count = 8);

} // namespace ParticleEmitters
} // namespace alx
