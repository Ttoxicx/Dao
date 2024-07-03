#pragma once

#include <atomic>
#include <limits>

namespace Dao {

	using ParticleEmitterID = std::size_t;
	
	constexpr ParticleEmitterID k_invalid_particle_emmiter_id = std::numeric_limits<std::size_t>::max();

	class ParticleEmitterIDAllocator {
	public:
		static ParticleEmitterID allocate();
		static void reset();

	private:
		static std::atomic<ParticleEmitterID> m_next_id;
	};
}