#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <3dmath.h>

struct Mesh
{
    const uint32_t vertexArrayBufferObject;
    const int numIndices;

    struct Bone
    {
        uint32_t index;
        uint32_t parentIndex;
        std::string name;
        glm::mat4x4 bindPose, invBindPose;
    };
    std::map<std::string, int> boneNamesMap;

    std::vector<Bone> bones;

    Mesh(uint32_t vertexArrayBufferObject, int numIndices) :
        vertexArrayBufferObject(vertexArrayBufferObject),
        numIndices(numIndices)
    {}
};

using MeshPtr = std::shared_ptr<Mesh>;

MeshPtr load_mesh(const char* path, int idx);
MeshPtr make_plane_mesh();
MeshPtr make_mesh(const std::vector<uint32_t>& indices, const std::vector<vec3>& vertices, const std::vector<vec3>& normals);

void render(const MeshPtr& mesh);
void render(const MeshPtr& mesh, int count);