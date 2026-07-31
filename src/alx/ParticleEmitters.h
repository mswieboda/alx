#pragma once

namespace alx {

class ParticleSystem;

namespace ParticleEmitters {

void spawn_hit_sparks(ParticleSystem& ps, float x, float y, int count = 10);
void spawn_spire_embers(ParticleSystem& ps, float x, float y, int count = 2);
void spawn_alloy_pickup(ParticleSystem& ps, float x, float y, int count = 8);

} // namespace ParticleEmitters
} // namespace alx
