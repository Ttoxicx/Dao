#include "runtime/function/framework/component/particle/particle_component.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/particle/particle_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_system.h"
#include "runtime/core/base/macro.h"

namespace Dao {
	void ParticleComponent::postLoadResource(std::weak_ptr<GObject> parent_object) {
		m_parent_object = parent_object;
		std::shared_ptr<ParticleManager> particle_manager = g_runtime_global_context.m_particle_manager;
		ASSERT(particle_manager);
		_local_transform.makeTransform(_particle_res.m_local_translation, Vector3::UNIT_SCALE, _particle_res.m_local_rotation);
		computeGlobalTransform();
		particle_manager->createParticleEmitter(_particle_res, _transform_desc);
	}

	void ParticleComponent::computeGlobalTransform() {
		TransformComponent* transform_component = m_parent_object.lock()->tryGetComponent(TransformComponent);
		Matrix4x4 globa_transform_matrix = transform_component->getMatrix() * _local_transform;
		Vector3 position, scale;
		Quaternion rotation;
		globa_transform_matrix.decomposition(position, scale, rotation);
		memcpy(_transform_desc.m_position.ptr(), position.ptr(), sizeof(float) * 3);
		rotation.toRotationMatrix(_transform_desc.m_rotation);
	}

	void ParticleComponent::tick(float delta_time) {
		RenderSwapContext& swap_context = g_runtime_global_context.m_render_system->getSwapContext();
		RenderSwapData& logic_swap_data = swap_context.getLogicSwapData();
		logic_swap_data.addTickParticleEmitter(_transform_desc.m_id);
		TransformComponent* transform_component = m_parent_object.lock()->tryGetComponent(TransformComponent);
		if (transform_component->isDirty()) {
			computeGlobalTransform();
			logic_swap_data.updateParticleTransform(_transform_desc);
		}
	}
}