#pragma once

#include "runtime/resource/res_type/components/animation.h"
#include "runtime/function/animation/bone.h"

namespace Dao {

	class SkeletonData;
	class BlendStateWithClipData;

	class Skeleton {
	public:
		~Skeleton();
		void            buildSkeleton(const SkeletonData& skeleton_definition);
		void            applyAnimation(const BlendStateWithClipData& blend_state);
		AnimationResult outputAnimationResult();
		void            resetSkeleton();
		const Bone* getBones() const;
		int32_t         getBonesCount() const;

	private:
		bool _is_flat{ false };
		int _bone_count{ 0 };
		Bone* _bones{ nullptr };
	};
}