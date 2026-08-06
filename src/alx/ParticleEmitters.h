#pragma once
#include "alx/Layer.h"

namespace alx {

class ParticleSystem;

namespace ParticleEmitters {

void spawn_sword_slash_trail(ParticleSystem& ps, float prev_tip_x, float prev_tip_y, float curr_tip_x, float curr_tip_y, float swing_progress, int count = 16, int z_index = Layer::WorldObjFX, int y_sort_override = -2147483647 - 1);
void spawn_hit_blood(ParticleSystem& ps, float x, float y, float kb_vx = 0.0f, float kb_vy = 0.0f, int count = 20, int z_index = Layer::WorldObjFX, int y_sort_override = -2147483647 - 1, float momentum_scale = 0.85f);
void spawn_spire_embers(ParticleSystem& ps, float x, float y, int count = 1, int z_index = 8000, int y_sort_override = -2147483647 - 1);
void spawn_refiner_embers(ParticleSystem& ps, float x, float y, int count = 1, int z_index = 8000, int y_sort_override = -2147483647 - 1);
void spawn_alloy_pickup(ParticleSystem& ps, float x, float y, int count = 15, int z_index = 10, int y_sort_override = -2147483647 - 1);
void spawn_straight_pipe_mana(ParticleSystem& ps, int tile_x, int tile_y, int dir_x, int dir_y, float sim_tick_rate, int count = 1, int tile_size = 16);
void spawn_corner_pipe_mana(ParticleSystem& ps, int tile_x, int tile_y, int in_dx, int in_dy, int out_dx, int out_dy, float sim_tick_rate, int count = 1, int tile_size = 16);
void spawn_tower_pulse(ParticleSystem& ps, float x, float y, float radius, int z_index = Layer::WorldObjFX, int y_sort_override = -2147483647 - 1);
void spawn_tower_shatter(ParticleSystem& ps, float x, float y, int z_index = Layer::WorldObjFX, int y_sort_override = -2147483647 - 1);
void spawn_egg_hatch(ParticleSystem& ps, float x, float y, int z_index = Layer::WorldObjFX, int y_sort_override = -2147483647 - 1);
void spawn_egg_shatter(ParticleSystem& ps, float x, float y, int z_index = Layer::WorldObjFX, int y_sort_override = -2147483647 - 1);
void spawn_dark_mana_spill(ParticleSystem& ps, float x, float y, int z_index = Layer::WorldObjFX, int y_sort_override = -2147483647 - 1);

} // namespace ParticleEmitters
} // namespace alx
