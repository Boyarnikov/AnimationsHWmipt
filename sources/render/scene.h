#pragma once

#include "mesh.h"
#include "skeleton.h"

struct SceneAsset
{
    std::vector<MeshPtr> meshes;
    SkeletonPtr skeleton;
    enum LoadScene
    {
        Meshes = 1 << 0,
        Skeleton = 1 << 1,
        Animation = 1 << 2
    };
};

SceneAsset load_scene(const char* path, int load_flags);
