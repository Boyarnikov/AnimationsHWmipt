#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/skeleton_builder.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/skeleton_utils.h"

#include <assimp/scene.h>
#include "render/scene.h"
#include "log.h"

void build_skeleton(ozz::animation::offline::RawSkeleton::Joint& root, const aiNode& ai_root)
{
    root.name = ai_root.mName.C_Str();

    aiVector3D scaling;
    aiQuaternion rotation;
    aiVector3D position;
    ai_root.mTransformation.Decompose(scaling, rotation, position);
    root.transform.translation = ozz::math::Float3(position.x, position.y, position.z);
    root.transform.rotation = ozz::math::Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
    root.transform.scale = ozz::math::Float3(scaling.x, scaling.y, scaling.z);

    root.children.resize(ai_root.mNumChildren);

    for (size_t i = 0; i < ai_root.mNumChildren; i++)
        build_skeleton(root.children[i], *ai_root.mChildren[i]);
}

SkeletonPtr create_skeleton(const aiNode& ai_root)
{
    // Creates a RawSkeleton.
    ozz::animation::offline::RawSkeleton raw_skeleton;

    // Creates the root joint.
    raw_skeleton.roots.resize(1);
    build_skeleton(raw_skeleton.roots[0], ai_root);

    if (!raw_skeleton.Validate())
    {
        debug_error("skeleton validation failed");
        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////////
    // This final section converts the RawSkeleton to a runtime Skeleton.
    //////////////////////////////////////////////////////////////////////////////

    // Creates a SkeletonBuilder instance.
    ozz::animation::offline::SkeletonBuilder builder;

    // Executes the builder on the previously prepared RawSkeleton, which returns
    // a new runtime skeleton instance.
    // This operation will fail and return an empty unique_ptr if the RawSkeleton
    // isn't valid.
    ozz::unique_ptr<ozz::animation::Skeleton> skeleton = builder(raw_skeleton);

    return std::shared_ptr<ozz::animation::Skeleton>(std::move(skeleton));
}

AnimationPtr create_animation(const aiAnimation& ai_animation, const SkeletonPtr& skeleton)
{

    ozz::animation::offline::RawAnimation raw_animation;

    // Sets animation duration (to 1.4s).
    // All the animation keyframes times must be within range [0, duration].
    double secondsPerTick = 1.f / ai_animation.mTicksPerSecond;
    raw_animation.duration = ai_animation.mDuration * secondsPerTick;
    raw_animation.name = ai_animation.mName.C_Str();

    raw_animation.tracks.resize(skeleton->num_joints());

    for (int jointIdx = 0; jointIdx < skeleton->num_joints(); jointIdx++)
    {
        ozz::animation::offline::RawAnimation::JointTrack& track = raw_animation.tracks[jointIdx];
        int channelIdx = -1;
        for (size_t i = 0; i < ai_animation.mNumChannels; i++)
            if (strcmp(ai_animation.mChannels[i]->mNodeName.C_Str(), skeleton->joint_names()[jointIdx]) == 0)
            {
                channelIdx = i;
                break;
            }
        if (channelIdx >= 0)
        {
            const aiNodeAnim& channel = *ai_animation.mChannels[channelIdx];

            track.translations.resize(channel.mNumPositionKeys);
            for (size_t i = 0; i < channel.mNumPositionKeys; i++)
            {

                track.translations[i] =
                    ozz::animation::offline::RawAnimation::TranslationKey{
                        float(channel.mPositionKeys[i].mTime * secondsPerTick), ozz::math::Float3(
                                                                                    channel.mPositionKeys[i].mValue.x,
                                                                                    channel.mPositionKeys[i].mValue.y,
                                                                                    channel.mPositionKeys[i].mValue.z) };
            }

            track.rotations.resize(channel.mNumRotationKeys);
            for (size_t i = 0; i < channel.mNumRotationKeys; i++)
            {

                track.rotations[i] =
                    ozz::animation::offline::RawAnimation::RotationKey{
                        float(channel.mRotationKeys[i].mTime * secondsPerTick), ozz::math::Quaternion(
                                                                                    channel.mRotationKeys[i].mValue.x,
                                                                                    channel.mRotationKeys[i].mValue.y,
                                                                                    channel.mRotationKeys[i].mValue.z,
                                                                                    channel.mRotationKeys[i].mValue.w) };
            }

            track.scales.resize(channel.mNumScalingKeys);
            for (size_t i = 0; i < channel.mNumScalingKeys; i++)
            {

                track.scales[i] =
                    ozz::animation::offline::RawAnimation::ScaleKey{
                        float(channel.mScalingKeys[i].mTime * secondsPerTick), ozz::math::Float3(
                                                                                   channel.mScalingKeys[i].mValue.x,
                                                                                   channel.mScalingKeys[i].mValue.y,
                                                                                   channel.mScalingKeys[i].mValue.z) };
            }
        }
        else
        {
            ozz::math::Transform jointTm = ozz::animation::GetJointLocalRestPose(*skeleton, jointIdx);

            track.translations =
            { ozz::animation::offline::RawAnimation::TranslationKey{0.f, jointTm.translation} };
            track.rotations =
            { ozz::animation::offline::RawAnimation::RotationKey{0.f, jointTm.rotation} };
            track.scales =
            { ozz::animation::offline::RawAnimation::ScaleKey{0.f, jointTm.scale} };
        }
    }

    // Test for animation validity. These are the errors that could invalidate
    // an animation:
    //  1. Animation duration is less than 0.
    //  2. Keyframes' are not sorted in a strict ascending order.
    //  3. Keyframes' are not within [0, duration] range.
    if (!raw_animation.Validate())
    {
        debug_error("animation validation failed");
        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////////
    // This final section converts the RawAnimation to a runtime Animation.
    //////////////////////////////////////////////////////////////////////////////

    // Creates a AnimationBuilder instance.
    ozz::animation::offline::AnimationBuilder builder;

    // Executes the builder on the previously prepared RawAnimation, which returns
    // a new runtime animation instance.
    // This operation will fail and return an empty unique_ptr if the RawAnimation
    // isn't valid.
    ozz::unique_ptr<ozz::animation::Animation> animation = builder(raw_animation);

    // ...use the animation as you want...

    debug_log("animation %s raw size = %d, runtime size = %d", animation->name(), raw_animation.size(), animation->size());

    std::fflush(stdout);
    return std::shared_ptr<ozz::animation::Animation>(std::move(animation));
}
