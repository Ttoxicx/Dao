#pragma once

#include "runtime/function/framework/component/component.h"
#include "runtime/function/particle/particle_desc.h"
#include "runtime/resource/res_type/components/emitter.h"
#include "runtime/core/math/matrix4.h"
#include "runtime/core/math/transform.h"

namespace Dao {

	REFLECTION_TYPE(ParticleComponent);
	CLASS(ParticleComponent:public Component, WhiteListFields)
	{
		REFLECTION_BODY(ParticleComponent);
	public:
		ParticleComponent() {}

		void postLoadResource(std::weak_ptr<GObject> parent_object) override;

		void tick(float delta_time) override;

	private:
		void computeGlobalTransform();

		META(Enable) ParticleComponentRes _particle_res;
		Matrix4x4 _local_transform;
		ParticleEmitterTransformDesc _transform_desc;
	};
}