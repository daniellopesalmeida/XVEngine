#include "App.h"
#include <renderer/Vertex.h>
#include <renderer/Material.h>
#include <core/Transform.h>
#include <utils/Logger.h>
#include <glm/glm.hpp>

App::App(Engine& engine)
    : m_engine(engine)
{
    engine.SetUpdateCallback([this](float dt) { Update(dt); });
    engine.SetFixedUpdateCallback([this](float dt) { FixedUpdate(dt); });
}

void App::Load()
{
    auto& sm = m_engine.GetSceneManager();
    auto* scene = sm.AddScene("render scene");

    // ── Lighting ────────────────────────────────────────────────────────────
    scene->lightDir = glm::normalize(glm::vec3(0.5f, 1.f, 0.5f));
    scene->lightColor = { 1.f, 0.98f, 0.9f };
    scene->ambientStrength = 0.55f;
    scene->specularStrength = 0.4f;
    scene->shininess = 32.f;

    // ── Geometry + Materials ─────────────────────────────────────────────────
    loadObj(scene);
    //loadGround(scene);
    //loadCube(scene);

    sm.LoadScene("render scene");
    Logger::Info("Scene loaded");
}

void App::Update(float deltaTime)
{
    auto* scene = m_engine.GetSceneManager().GetActiveScene();
    if (!scene || m_Objects.empty()) return;

    for (auto& [name, handle] : m_Objects)
    {
        auto& obj = scene->GetObject(handle);
        //obj.transform.rotation.y += 45.f * deltaTime;
        //Logger::Info("rotation.y = ", obj.transform.rotation.y);
    }
}

void App::FixedUpdate(float deltaTime)
{
    // physics later
}

void App::loadObj(Scene* scene)
{
    // Load meshes
    scene->LoadMesh("models/vehicle.obj", "vehicle");
    scene->LoadMesh("models/fireFX.obj", "fire");

    // Load materials — provide paths to each texture slot.
    // Leave a slot empty ("") to use the built-in fallback texture.
    scene->LoadMaterial("vehicleMat", MaterialDesc{
        .diffuse = "textures/vehicle_diffuse.png",
        .specular = "textures/vehicle_specular.png",
        .gloss = "textures/vehicle_gloss.png",
        .normal = "textures/vehicle_normal.png",
        });

    scene->LoadMaterial("fireMat", MaterialDesc{
        .diffuse = "textures/fireFX_diffuse.png",
        // no specular / gloss / normal — fallbacks used automatically
        });

    // Add objects — mesh name + transform + material name
    m_Objects["vehicle"] = scene->AddObject("vehicle",
        Transform{ .position = { 0.f, 0.f, -50.f } },
        "vehicleMat");

    m_Objects["fire"] = scene->AddObject("fire",
        Transform{ .position = { 0.f, 0.f, -50.f } },
        "fireMat");
}

void App::loadCube(Scene* scene)
{
    // tangent is now vec4: xyz = direction, w = handedness (+1 for non-mirrored faces)
    const std::vector<Vertex> verts =
    {
        // FRONT (red)   pos                    color    normal     uv      tangent
        {{-0.5f, -0.5f,  0.5f}, {1,0,0}, {0,0,1}, {0,0}, {1,0,0,1}},
        {{ 0.5f, -0.5f,  0.5f}, {1,0,0}, {0,0,1}, {1,0}, {1,0,0,1}},
        {{ 0.5f,  0.5f,  0.5f}, {1,0,0}, {0,0,1}, {1,1}, {1,0,0,1}},
        {{-0.5f,  0.5f,  0.5f}, {1,0,0}, {0,0,1}, {0,1}, {1,0,0,1}},

        // BACK (green)
        {{ 0.5f, -0.5f, -0.5f}, {0,1,0}, {0,0,-1}, {0,0}, {-1,0,0,1}},
        {{-0.5f, -0.5f, -0.5f}, {0,1,0}, {0,0,-1}, {1,0}, {-1,0,0,1}},
        {{-0.5f,  0.5f, -0.5f}, {0,1,0}, {0,0,-1}, {1,1}, {-1,0,0,1}},
        {{ 0.5f,  0.5f, -0.5f}, {0,1,0}, {0,0,-1}, {0,1}, {-1,0,0,1}},

        // RIGHT (blue)
        {{ 0.5f, -0.5f,  0.5f}, {0,0,1}, {1,0,0}, {0,0}, {0,0,-1,1}},
        {{ 0.5f, -0.5f, -0.5f}, {0,0,1}, {1,0,0}, {1,0}, {0,0,-1,1}},
        {{ 0.5f,  0.5f, -0.5f}, {0,0,1}, {1,0,0}, {1,1}, {0,0,-1,1}},
        {{ 0.5f,  0.5f,  0.5f}, {0,0,1}, {1,0,0}, {0,1}, {0,0,-1,1}},

        // LEFT (yellow)
        {{-0.5f, -0.5f, -0.5f}, {1,1,0}, {-1,0,0}, {0,0}, {0,0,1,1}},
        {{-0.5f, -0.5f,  0.5f}, {1,1,0}, {-1,0,0}, {1,0}, {0,0,1,1}},
        {{-0.5f,  0.5f,  0.5f}, {1,1,0}, {-1,0,0}, {1,1}, {0,0,1,1}},
        {{-0.5f,  0.5f, -0.5f}, {1,1,0}, {-1,0,0}, {0,1}, {0,0,1,1}},

        // TOP (magenta)
        {{-0.5f,  0.5f,  0.5f}, {1,0,1}, {0,1,0}, {0,0}, {1,0,0,1}},
        {{ 0.5f,  0.5f,  0.5f}, {1,0,1}, {0,1,0}, {1,0}, {1,0,0,1}},
        {{ 0.5f,  0.5f, -0.5f}, {1,0,1}, {0,1,0}, {1,1}, {1,0,0,1}},
        {{-0.5f,  0.5f, -0.5f}, {1,0,1}, {0,1,0}, {0,1}, {1,0,0,1}},

        // BOTTOM (cyan)
        {{-0.5f, -0.5f, -0.5f}, {0,1,1}, {0,-1,0}, {0,0}, {1,0,0,1}},
        {{ 0.5f, -0.5f, -0.5f}, {0,1,1}, {0,-1,0}, {1,0}, {1,0,0,1}},
        {{ 0.5f, -0.5f,  0.5f}, {0,1,1}, {0,-1,0}, {1,1}, {1,0,0,1}},
        {{-0.5f, -0.5f,  0.5f}, {0,1,1}, {0,-1,0}, {0,1}, {1,0,0,1}},
    };

    const std::vector<uint32_t> indices =
    {
        0,1,2,   2,3,0,
        4,5,6,   6,7,4,
        8,9,10,  10,11,8,
        12,13,14,14,15,12,
        16,17,18,18,19,16,
        20,21,22,22,23,20
    };

    m_Objects["cube"] = scene->AddObject(
        verts, indices,
        Transform{ .position = { 0.f, 0.f, 0.f } },
        "cube");
}

void App::loadQuad(Scene* scene)
{
    const std::vector<Vertex> verts = {
        {{ -0.5f, -0.5f, 0.f }, { 1.f, 0.f, 0.f }, { 0.f, 0.f, 1.f }, {0.f, 0.f}, {1,0,0,1}},
        {{  0.5f, -0.5f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }, {1.f, 0.f}, {1,0,0,1}},
        {{  0.5f,  0.5f, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 0.f, 1.f }, {1.f, 1.f}, {1,0,0,1}},
        {{ -0.5f,  0.5f, 0.f }, { 1.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }, {0.f, 1.f}, {1,0,0,1}},
    };
    const std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

    m_Objects["quad"] = scene->AddObject(verts, indices,
        Transform{ .position = { 0.f, 0.f, 0.f } }, "quad");
}

void App::loadGround(Scene* scene)
{
    constexpr float size = 1000.0f;

    const std::vector<Vertex> verts =
    {
        // positions                     // color      // normal   // uv    // tangent (w=handedness)
        {{-size, -10.f, -size}, {1,1,1}, {0,1,0}, {0,0}, {1,0,0,1}},
        {{ size, -10.f, -size}, {1,1,1}, {0,1,0}, {1,0}, {1,0,0,1}},
        {{ size, -10.f,  size}, {1,1,1}, {0,1,0}, {1,1}, {1,0,0,1}},
        {{-size, -10.f,  size}, {1,1,1}, {0,1,0}, {0,1}, {1,0,0,1}},
    };

    const std::vector<uint32_t> indices =
    {
        0,3,2,
        2,1,0
    };

    m_Objects["ground"] = scene->AddObject(
        verts,
        indices,
        Transform{},
        "groundMat");
}