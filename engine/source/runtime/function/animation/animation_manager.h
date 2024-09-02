#pragma once

#include "runtime/resource/res_type/data/animation_clip.h"
#include "runtime/resource/res_type/data/animation_skeleton_node_map.h"
#include "runtime/resource/res_type/data/blend_state.h"
#include "runtime/resource/res_type/data/skeleton_data.h"
#include "runtime/resource/res_type/data/skeleton_mask.h"

#include <map>
#include <memory>
#include <string>

namespace Dao {

    class AnimationManager {
    public:
        static std::shared_ptr<SkeletonData>  tryLoadSkeleton(std::string file_path);
        static std::shared_ptr<AnimationClip> tryLoadAnimation(std::string file_path);
        static std::shared_ptr<AnimSkelMap>   tryLoadAnimationSkeletonMap(std::string file_path);
        static std::shared_ptr<BoneBlendMask> tryLoadSkeletonMask(std::string file_path);
        static BlendStateWithClipData getBlendStateWithClipData(const BlendState& blend_state);

        AnimationManager() = default;

    private:
        static std::map<std::string, std::shared_ptr<SkeletonData>> _skeleton_definition_cache;
        static std::map<std::string, std::shared_ptr<AnimationClip>> _animation_data_cache;
        static std::map<std::string, std::shared_ptr<AnimSkelMap>> _animation_skeleton_map_cache;
        static std::map<std::string, std::shared_ptr<BoneBlendMask>> _skeleton_mask_cache;
    };
}