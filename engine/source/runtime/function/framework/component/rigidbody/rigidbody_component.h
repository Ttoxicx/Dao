#pragma once


#include "runtime/core/math/transform.h"
#include "runtime/core/math/axis_aligned.h"
#include "runtime/resource/res_type/components/rigid_body.h"
#include "runtime/function/framework/component/component.h"

namespace Dao {

	REFLECTION_TYPE(RigidBodyComponent);
	CLASS(RigidBodyComponent:public Component, WhiteListFields)
	{
		REFLECTION_BODY(RigidBodyComponent);
	public:
		RigidBodyComponent() = default;
		~RigidBodyComponent() override;

		void postLoadResource(std::weak_ptr<GObject> parent_object) override;

		void tick(float delta_time) override {}

		void updateGlobalTransform(const Transform & transform, bool is_scale_dirty);
		void getShapeBoundingBoxes(std::vector<AxisAlignedBox>&bounding_boxes) const;

	protected:
		void createRigidBody(const Transform & global_transform);
		void removeRigidBody();

		META(Enable) RigidBodyComponentRes m_rigidbody_res;
		uint32_t m_rigidbody_id{ 0xFFFFFFFF };
	};
}