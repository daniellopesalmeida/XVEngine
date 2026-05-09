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
    //camera — filled by Scene::BuildRenderList
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 cameraPos;

    //directional light — set by App before BuildRenderList
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.f, -1.f, -0.5f));
    glm::vec3 lightColor = { 1.f, 1.f, 1.f };
    float ambientStrength = 0.15f;
    float specularStrength = 0.5f;
    float shininess = 32.f;

    std::vector<DrawCall> drawCalls;

    void Clear() { drawCalls.clear(); }
    void Add(const DrawCall& dc) { drawCalls.push_back(dc); }
};