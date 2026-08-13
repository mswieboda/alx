#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "alx/Particle.h"

namespace alx {

struct Camera;

class ParticleSystem {
public:
    static constexpr size_t POOL_CAPACITY = 1024;
    static constexpr int PARTICLE_Z_INDEX = 8000;

    ParticleSystem();

    /// Emits a new particle by claiming an inactive slot in O(1) time.
    Particle* emit();

    /// Deactivates all active particles and resets the free-list index stack in O(1) setup time.
    void clear();

    /// Updates lifetime timers, particle movement, and recycles dead particles in O(1) time.
    void update(float dt);

    /// Enqueues draw commands for active particles with camera viewport culling.
    void draw(const Camera* camera = nullptr) const;

    /// Returns the number of currently active particles in O(1) constant time.
    [[nodiscard]] size_t active_count() const { return m_active_count; }

    [[nodiscard]] const std::array<Particle, POOL_CAPACITY>& pool() const { return m_pool; }
    [[nodiscard]] std::array<Particle, POOL_CAPACITY>& pool() { return m_pool; }

private:
    void init_free_list();

    std::array<Particle, POOL_CAPACITY> m_pool{};
    std::array<uint16_t, POOL_CAPACITY> m_free_slots{};
    size_t m_free_count{POOL_CAPACITY};
    size_t m_active_count{0};
    size_t m_next_slot{0};
};

} // namespace alx

