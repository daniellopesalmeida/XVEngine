#pragma once
#include <glm/glm.hpp>
#include <vector>

struct DrawCall
{
    glm::mat4 transform;  //model matrix
    struct Mesh* mesh;    //non-owning
};

struct RenderList
{
    glm::mat4 view;
    glm::mat4 proj;
    std::vector<DrawCall> drawCalls;

    void Clear() { drawCalls.clear(); }
    void Add(const DrawCall& dc) { drawCalls.push_back(dc); }
};