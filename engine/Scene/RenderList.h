#pragma once
#include <glm/glm.hpp>
#include <scene/Light.h>
#include <vector>

struct DrawCall
{
    glm::mat4        transform;   // model matrix
    struct Mesh* mesh;        // non-owning
    struct Material* material;    // non-owning — nullptr = use default/no-texture pipeline
};

struct RenderList
{
    // ── Camera — filled by Scene::BuildRenderList ─────────────────────────────
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 cameraPos;

    // ── Ambient — scene-wide, not per-light ──────────────────────────────────
    glm::vec3 ambientColor = { 1.f, 1.f, 1.f };
    float     ambientStrength = 0.15f;

    // ── Lights — copied from Scene::m_lights each frame ──────────────────────
    std::vector<Light> lights;

    // ── Draw calls ───────────────────────────────────────────────────────────
    std::vector<DrawCall> drawCalls;

    void Clear()
    {
        drawCalls.clear();
        lights.clear();
    }

    void Add(const DrawCall& dc) { drawCalls.push_back(dc); }
    void Add(const Light& light) { lights.push_back(light); }
};