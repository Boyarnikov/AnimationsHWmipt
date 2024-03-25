#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <3dmath.h>

struct Skeleton
{
	size_t nodeCount = 0;
	std::vector<std::string> names;
	std::vector<glm::mat4> localTm;
	std::vector<int> parent;
	std::map<std::string, int> nodeNameToIdx;
};

using SkeletonPtr = std::shared_ptr<const Skeleton>;

struct RuntimeSkeleton
{
	SkeletonPtr ref;
	std::vector<glm::mat4> localTm;
	std::vector<glm::mat4> globalTm;
	RuntimeSkeleton(SkeletonPtr ref) : ref(ref), localTm(ref->localTm), globalTm(ref->nodeCount) {}

	void updateLocalTransforms();
};
