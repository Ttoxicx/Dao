#include "runtime/function/animation/skeleton.h"

#include "runtime/core/math/math.h"
#include "runtime/function/animation/utilities.h"

namespace Dao {

    Skeleton::~Skeleton() {
        delete[] _bones;
    }

    void Skeleton::resetSkeleton() {
        for (size_t i = 0; i < _bone_count; ++i) {
            _bones[i].resetToInitialPose();
        }
    }

    void Skeleton::buildSkeleton(const SkeletonData& skeleton_definition) {
        _is_flat = skeleton_definition.m_is_flat;
        if (_bones != nullptr) {
            delete[] _bones;
        }
        if (!_is_flat || !skeleton_definition.m_in_topological_order) {
            // LOG_ERROR
            return;
        }
        _bone_count = skeleton_definition.m_bones_map.size();
        _bones = new Bone[_bone_count];
        for (size_t i = 0; i < _bone_count; ++i) {
            const RawBone bone_definition = skeleton_definition.m_bones_map[i];
            Bone* parent_bone = find_by_index(_bones, bone_definition.m_parent_index, i, _is_flat);
            _bones[i].initialize(std::make_shared<RawBone>(bone_definition), parent_bone);
        }
    }

    void Skeleton::applyAnimation(const BlendStateWithClipData& blend_state) {
        if (!_bones) {
            return;
        }
        resetSkeleton();
        for (size_t clip_index = 0; clip_index < 1; ++clip_index) {
            const AnimationClip& animation_clip = blend_state.m_blend_clip[clip_index];
            const float phase = blend_state.m_blend_ratio[clip_index];
            const AnimSkelMap& anim_skel_map = blend_state.m_blend_anim_skel_map[clip_index];

            float exact_frame = phase * (animation_clip.m_total_frame - 1);
            int current_frame_low = floor(exact_frame);
            int current_frame_high = ceil(exact_frame);
            float lerp_ratio = exact_frame - current_frame_low;
            for (size_t node_index = 0; node_index < animation_clip.m_node_count && node_index < anim_skel_map.m_convert.size(); ++node_index) {
                AnimationChannel channel = animation_clip.m_node_channels[node_index];
                size_t bone_index = anim_skel_map.m_convert[node_index];
                float weight = 1; // blend_state.blend_weight[clip_index]->blend_weight[bone_index];
                weight = 1;
                if (fabs(weight) < 0.0001f) {
                    continue;
                }
                if (bone_index == std::numeric_limits<size_t>().max()) {
                    // LOG_WARNING
                    continue;
                }
                Bone* bone = &_bones[bone_index];
                if (channel.m_position_keys.size() <= current_frame_high) {
                    current_frame_high = channel.m_position_keys.size() - 1;
                }
                if (channel.m_scaling_keys.size() <= current_frame_high) {
                    current_frame_high = channel.m_scaling_keys.size() - 1;
                }
                if (channel.m_rotation_keys.size() <= current_frame_high) {
                    current_frame_high = channel.m_rotation_keys.size() - 1;
                }
                current_frame_low = (current_frame_low < current_frame_high) ? current_frame_low : current_frame_high;
                Vector3 position = Vector3::lerp(channel.m_position_keys[current_frame_low], channel.m_position_keys[current_frame_high], lerp_ratio);
                Vector3 scaling = Vector3::lerp(channel.m_scaling_keys[current_frame_low], channel.m_scaling_keys[current_frame_high], lerp_ratio);
                Quaternion rotation = Quaternion::nLerp(lerp_ratio, channel.m_rotation_keys[current_frame_low], channel.m_rotation_keys[current_frame_high], true);
                bone->rotate(rotation);
                bone->scale(scaling);
                bone->translate(position);
            }
        }
        for (size_t i = 0; i < _bone_count; ++i) {
            _bones[i].update();
        }
    }

    AnimationResult Skeleton::outputAnimationResult() {
        AnimationResult animation_result;
        for (size_t i = 0; i < _bone_count; ++i) {
            std::shared_ptr<AnimationResultElement> animation_result_element = std::make_shared<AnimationResultElement>();
            Bone* bone = &_bones[i];
            animation_result_element->m_index = bone->getID() + 1;
            Vector3 temp_translation = bone->getDerivedPosition();

            // TODO(the unit of the joint matrices is wrong)
            Vector3 temp_scale = bone->getDerivedScale();

            Quaternion temp_rotation = bone->getDerivedOrientation();
            auto objMat = Transform(bone->getDerivedPosition(), bone->getDerivedOrientation(), bone->getDerivedScale()).getMatrix();

            auto resMat = objMat * bone->getInverseTpose();

            animation_result_element->m_transform = resMat.toMatrix4x4_();

            animation_result.m_node.push_back(*animation_result_element);
        }
        return animation_result;
    }

    const Bone* Skeleton::getBones() const {
        return _bones;
    }

    int32_t Skeleton::getBonesCount() const {
        return _bone_count;
    }
}