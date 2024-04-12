#include "scene.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <log.h>

MeshPtr create_mesh(const aiMesh* mesh);

SkeletonPtr create_skeleton(const aiNode& ai_node);
AnimationPtr create_animation(const aiAnimation& ai_animation, const SkeletonPtr& skeleton);

SceneAsset load_scene(const char* path, int load_flags)
{
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.f);
    importer.ReadFile(path, aiPostProcessSteps::aiProcess_Triangulate | aiPostProcessSteps::aiProcess_LimitBoneWeights |

        aiPostProcessSteps::aiProcess_GenNormals | aiProcess_GlobalScale | aiProcess_FlipWindingOrder | aiProcess_PopulateArmatureData);

    const aiScene* scene = importer.GetScene();

    SceneAsset result;

    if (!scene)
    {
        debug_error("no asset in %s", path);
        return result;
    }

    if (load_flags & SceneAsset::LoadScene::Meshes)
    {
        result.meshes.reserve(scene->mNumMeshes);
        for (size_t i = 0; i < scene->mNumMeshes; i++)
        {
            result.meshes.emplace_back(create_mesh(scene->mMeshes[i]));
        }    
    }

    if (load_flags & SceneAsset::LoadScene::Skeleton)
    {
        result.skeleton = create_skeleton(*scene->mRootNode);
    }
    if (load_flags & SceneAsset::LoadScene::Animation && result.skeleton)
    {
        result.animations.reserve(scene->mNumAnimations);
        for (size_t i = 0; i < scene->mNumAnimations; i++)
            if (AnimationPtr animation = create_animation(*scene->mAnimations[i], result.skeleton))
                result.animations.emplace_back(std::move(animation));
    }

    importer.FreeScene();
    return result;
}
