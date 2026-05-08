
#pragma once
#include <glm/glm.hpp>

//per-draw push constant — model matrix only
//view + proj are now in the FrameData UBO (set 0, binding 0)
struct ObjectData
{
    glm::mat4 model;
};
static_assert(sizeof(ObjectData) <= 128, "ObjectData exceeds push constant minimum guarantee");