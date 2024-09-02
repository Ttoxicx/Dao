#pragma once

#include "runtime/function/animation/node.h"

namespace Dao {
	
	class Bone :public Node {
		friend class LoDSkeleton;

	public:
		Bone();
		void initialize(std::shared_ptr<RawBone> dinition, Bone* parent_bone);
		size_t getID() const;

	protected:
		std::shared_ptr<RawBone> m_definition{};
	};
}