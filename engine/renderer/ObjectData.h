#pragma once
#include <glm/glm.hpp>

// Per-draw push constant — 112 bytes, within the 128-byte guarantee.
// model       — transforms vertices to world space
// normalColN  — column N of the normal matrix (inverse-transpose of model's mat3)
//               packed as vec4 to satisfy alignment; shader reads .xyz
struct ObjectData
{
    glm::mat4 model;        // 64 bytes

    glm::vec4 normalCol0;   // column 0 of normal matrix
    glm::vec4 normalCol1;   // column 1 of normal matrix
    glm::vec4 normalCol2;   // column 2 of normal matrix
};
static_assert(sizeof(ObjectData) <= 128, "ObjectData exceeds 128-byte push constant guarantee");