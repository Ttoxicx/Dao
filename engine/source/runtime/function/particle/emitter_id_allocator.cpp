#include "runtime/function/particle/emitter_id_allocator.h"

#include "core/base/macro.h"

namespace Dao {

	std::atomic<ParticleEmitterID> ParticleEmitterIDAllocator::m_next_id{ 0 };

	ParticleEmitterID ParticleEmitterIDAllocator::allocate() {
		std::atomic<ParticleEmitterID> new_emmiter = m_next_id.load();
		m_next_id++;
		if (m_next_id >= k_invalid_particle_emmiter_id) {
			LOG_FATAL("particle emitter id overflow");
		}
		return new_emmiter;
	}

	void ParticleEmitterIDAllocator::reset() {
		m_next_id.store(0);
	}
}
