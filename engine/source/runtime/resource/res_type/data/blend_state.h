#pragma once

#include "runtime/core/meta/reflection/reflection.h"
#include "runtime/resource/res_type/data/animation_clip.h"
#include "runtime/resource/res_type/data/animation_skeleton_node_map.h"

#include <string>
#include <vector>

namespace Dao {

    REFLECTION_TYPE(BoneBlendWeight);
    CLASS(BoneBlendWeight, Fields)
    {
        REFLECTION_BODY(BoneBlendWeight);
    public:
        std::vector<float> blend_weight;
    };

    REFLECTION_TYPE(BlendStateWithClipData);
    CLASS(BlendStateWithClipData, Fields)
    {
        REFLECTION_BODY(BlendStateWithClipData);
    public:
        int                          m_clip_count;
        std::vector<AnimationClip>   m_blend_clip;
        std::vector<AnimSkelMap>     m_blend_anim_skel_map;
        std::vector<BoneBlendWeight> m_blend_weight;
        std::vector<float>           m_blend_ratio;
    };

    REFLECTION_TYPE(BlendState);
    CLASS(BlendState, Fields)
    {
        REFLECTION_BODY(BlendState);
    public:
        int                      m_clip_count;
        std::vector<std::string> m_blend_clip_file_path;
        std::vector<float>       m_blend_clip_file_length;
        std::vector<std::string> m_blend_anim_skel_map_path;
        std::vector<float>       m_blend_weight;
        std::vector<std::string> m_blend_mask_file_path;
        std::vector<float>       m_blend_ratio;
    };
}