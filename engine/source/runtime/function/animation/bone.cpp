#include "runtime/function/animation/bone.h"

namespace Dao {
	Bone::Bone() :Node(std::string()) {
	}

	void Bone::initialize(std::shared_ptr<RawBone> definition, Bone* parent_bone) {
		m_definition = definition;
		if (definition) {
			m_name = definition->m_name;
			setOrientation(definition->m_binding_pose.m_rotation);
			setPosition(definition->m_binding_pose.m_position);
			setScale(definition->m_binding_pose.m_scale);
			m_inverse_Tpose = definition->m_tpose_matrix;
			setAsInitialPose();
		}
		m_parent = parent_bone;
	}

	size_t Bone::getID() const {
		if (m_definition) {
			return m_definition->m_index;
		}
		return std::numeric_limits<size_t>().max();
	}
}