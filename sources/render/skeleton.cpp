#include "skeleton.h"
#include <assimp/scene.h>

static void iterate_over_tree(int parent_idx, const aiNode* node, Skeleton& skeleton)
{
    const aiMatrix4x4& transform = node->mTransformation;
    glm::mat4 localTm = glm::mat4(
        transform.a1, transform.b1, transform.c1, transform.d1,
        transform.a2, transform.b2, transform.c2, transform.d2,
        transform.a3, transform.b3, transform.c3, transform.d3,
        transform.a4, transform.b4, transform.c4, transform.d4);

    int nodeIdx = skeleton.nodeCount;
    skeleton.nodeCount++;

    skeleton.nodeNameToIdx.emplace(node->mName.C_Str(), nodeIdx);
    skeleton.names.emplace_back(node->mName.C_Str());
    skeleton.parent.emplace_back(parent_idx);
    skeleton.localTm.emplace_back(localTm);

    for (size_t i = 0; i < node->mNumChildren; i++)
    {
        iterate_over_tree(nodeIdx, node->mChildren[i], skeleton);
    }
}

SkeletonPtr load_skeleton(const aiNode* node)
{
    Skeleton skeleton;
    iterate_over_tree(-1, node, skeleton);

    return std::make_shared<Skeleton>(std::move(skeleton));
}

void RuntimeSkeleton::updateLocalTransforms()
{
    for (size_t i = 0; i < ref->nodeCount; i++)
    {
        int parent = ref->parent[i];
        globalTm[i] = (parent >= 0 ? globalTm[parent] : glm::mat4(1.f)) * localTm[i];
    }
}
