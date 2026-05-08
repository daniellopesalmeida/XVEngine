#include "App.h"
#include <renderer/Vertex.h>
#include <core/Transform.h>
#include <utils/Logger.h>

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

    //loadQuad(scene);
    //loadCube(scene);
    loadObj(scene);


    sm.LoadScene("render scene");
    Logger::Info("Scene loaded");
    
}

void App::Update(float deltaTime)
{
    auto* scene = m_engine.GetSceneManager().GetActiveScene();
    if (!scene || m_Objects.empty()) return;


    //rotate
    //scene->GetObject(m_Objects["vehicle"]).transform.rotation.y += 45.f * deltaTime;
    //scene->GetObject(m_Objects["fire"]).transform.rotation.y += 45.f * deltaTime;
    for (auto& [name, handle] : m_Objects)
    {
        auto& obj = scene->GetObject(handle);

        obj.transform.rotation.y += 45.f * deltaTime;
        //obj.transform.rotation.x += 20.f * deltaTime;
    }


}

void App::FixedUpdate(float deltaTime)
{
    //physics later
}

void App::loadCube(Scene* scene)
{
    const std::vector<Vertex> verts =
    {
        //FRONT (red)
        {{-0.5f, -0.5f,  0.5f}, {1,0,0}},
        {{ 0.5f, -0.5f,  0.5f}, {1,0,0}},
        {{ 0.5f,  0.5f,  0.5f}, {1,0,0}},
        {{-0.5f,  0.5f,  0.5f}, {1,0,0}},

        //BACK (green)
        {{ 0.5f, -0.5f, -0.5f}, {0,1,0}},
        {{-0.5f, -0.5f, -0.5f}, {0,1,0}},
        {{-0.5f,  0.5f, -0.5f}, {0,1,0}},
        {{ 0.5f,  0.5f, -0.5f}, {0,1,0}},

        //RIGHT (blue)
        {{ 0.5f, -0.5f,  0.5f}, {0,0,1}},
        {{ 0.5f, -0.5f, -0.5f}, {0,0,1}},
        {{ 0.5f,  0.5f, -0.5f}, {0,0,1}},
        {{ 0.5f,  0.5f,  0.5f}, {0,0,1}},

        //LEFT (yellow)
        {{-0.5f, -0.5f, -0.5f}, {1,1,0}},
        {{-0.5f, -0.5f,  0.5f}, {1,1,0}},
        {{-0.5f,  0.5f,  0.5f}, {1,1,0}},
        {{-0.5f,  0.5f, -0.5f}, {1,1,0}},

        //TOP (magenta)
        {{-0.5f,  0.5f,  0.5f}, {1,0,1}},
        {{ 0.5f,  0.5f,  0.5f}, {1,0,1}},
        {{ 0.5f,  0.5f, -0.5f}, {1,0,1}},
        {{-0.5f,  0.5f, -0.5f}, {1,0,1}},

        //BOTTOM (cyan)
        {{-0.5f, -0.5f, -0.5f}, {0,1,1}},
        {{ 0.5f, -0.5f, -0.5f}, {0,1,1}},
        {{ 0.5f, -0.5f,  0.5f}, {0,1,1}},
        {{-0.5f, -0.5f,  0.5f}, {0,1,1}},
    };

    const std::vector<uint32_t> indices =
    {
        0,1,2, 2,3,0,        // front
        4,5,6, 6,7,4,        // back
        8,9,10, 10,11,8,     // right
        12,13,14, 14,15,12,  // left
        16,17,18, 18,19,16,  // top
        20,21,22, 22,23,20   // bottom
    };

    m_Objects["cube"] = scene->AddObject(
        verts,
        indices,
        Transform{ .position = { 0.f, 0.f, 0.f } },
        "cube"
    );
}

void App::loadObj(Scene* scene)
{
    scene->LoadMesh("models/vehicle.obj", "vehicle");
    scene->LoadMesh("models/fireFX.obj", "fire");
    m_Objects["vehicle"] = scene->AddObject("vehicle", Transform{ .position = { 0.f, 0.f, -50.f } });
    m_Objects["fire"] = scene->AddObject("fire", Transform{ .position = { 0.f, 0.f, -50.f } });
}

void App::loadQuad(Scene* scene)
{
    const std::vector<Vertex> verts = {
        {{ -0.5f, -0.5f, 0.f }, { 1.f, 0.f, 0.f }},
        {{  0.5f, -0.5f, 0.f }, { 0.f, 1.f, 0.f }},
        {{  0.5f,  0.5f, 0.f }, { 0.f, 0.f, 1.f }},
        {{ -0.5f,  0.5f, 0.f }, { 1.f, 1.f, 0.f }},
    };
    const std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

    m_Objects["quad"] = scene->AddObject(verts, indices, Transform{.position = {0.f, 0.f, 0.f}}, "quad");
}
