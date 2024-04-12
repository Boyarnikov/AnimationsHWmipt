#pragma once

#include "mesh.h"

namespace ozz
{
    namespace animation
    {
        class Skeleton;
        class Animation;
    }
}

using SkeletonPtr = std::shared_ptr<ozz::animation::Skeleton>;
using AnimationPtr = std::shared_ptr<ozz::animation::Animation>;

struct SceneAsset
{
    std::vector<MeshPtr> meshes;
    SkeletonPtr skeleton;
    std::vector<AnimationPtr> animations;
    enum LoadScene
    {
        Meshes = 1 << 0,
        Skeleton = 1 << 1,
        Animation = 1 << 2
    };
};

SceneAsset load_scene(const char* path, int load_flags);
