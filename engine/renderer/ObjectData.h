
#pragma once
#include <glm/glm.hpp>

struct ObjectData
{
    glm::mat4 mvp;  //model * view * proj
};
static_assert(sizeof(ObjectData) <= 128, "ObjectData exceeds push constant minimum guarantee");