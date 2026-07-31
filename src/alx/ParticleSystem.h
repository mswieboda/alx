#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "alx/Particle.h"

namespace alx {

class ParticleSystem {
public:
    static constexpr size_t POOL_CAPACITY = 1024; // 256; // could even do 2048 if needed
    static constexpr int PARTICLE_Z_INDEX = 8000;

    ParticleSystem() = default;

    /// Emits a new particle by claiming an inactive slot, or overwriting via ring-buffer when full.
    Particle* emit();

    /// Deactivates all particles in the pool and resets ring-buffer pointer.
    void clear();

    /// Updates lifetime timers and particle movement.
    void update(float dt);

    /// Enqueues draw commands for all active particles with dynamic alpha falloff.
    void draw() const;

    /// Returns the number of currently active particles.
    [[nodiscard]] size_t active_count() const;

    [[nodiscard]] const std::array<Particle, POOL_CAPACITY>& pool() const { return m_pool; }
    [[nodiscard]] std::array<Particle, POOL_CAPACITY>& pool() { return m_pool; }

private:
    std::array<Particle, POOL_CAPACITY> m_pool{};
    size_t m_next_slot{0};
};

} // namespace alx
