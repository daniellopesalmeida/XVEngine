
#pragma once
#include <glm/glm.hpp>

//per-draw push constant
//model — transforms vertices to world space
//normalMatrix — inverse-transpose of model, upper-left 3x3
struct ObjectData
{
    glm::mat4 model;

    //mat3 as 3x vec4 — shader reads .xyz of each row
    glm::vec4 normalRow0;
    glm::vec4 normalRow1;
    glm::vec4 normalRow2;
};
static_assert(sizeof(ObjectData) <= 128, "ObjectData exceeds 128-byte push constant guarantee");