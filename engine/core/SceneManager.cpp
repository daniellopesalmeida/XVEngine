#include "SceneManager.h"
#include <utils/Logger.h>
#include <stdexcept>

void SceneManager::Init(Device& device, CommandManager& cmdManager,
    DescriptorManager& descriptorManager, float aspect)
{
    m_device = &device;
    m_cmdManager = &cmdManager;
    m_descMgr = &descriptorManager;
    m_aspect = aspect;
    Logger::Info("SceneManager initialized");
}

void SceneManager::Shutdown()
{
    UnloadCurrent();
    for (auto& [name, scene] : m_scenes)
        scene->Shutdown();
    m_scenes.clear();
    Logger::Info("SceneManager shutdown");
}

Scene* SceneManager::AddScene(const std::string& name)
{
    if (m_scenes.count(name))
        throw std::runtime_error("Scene already exists: " + name);

    auto scene = std::make_unique<Scene>();
    scene->Init(*m_device, *m_cmdManager, *m_descMgr, m_aspect);
    Scene* ptr = scene.get();
    m_scenes[name] = std::move(scene);

    Logger::Info("Scene added: ", name);
    return ptr;
}

void SceneManager::LoadScene(const std::string& name)
{
    auto it = m_scenes.find(name);
    if (it == m_scenes.end())
        throw std::runtime_error("Scene not found: " + name);

    UnloadCurrent();
    m_activeScene = it->second.get();
    m_activeSceneName = name;
    Logger::Info("Scene loaded: ", name);
}

void SceneManager::UnloadCurrent()
{
    if (!m_activeScene) return;
    Logger::Info("Scene unloaded: ", m_activeSceneName);
    m_activeScene = nullptr;
    m_activeSceneName = {};
}

Scene* SceneManager::GetScene(const std::string& name)
{
    auto it = m_scenes.find(name);
    return it == m_scenes.end() ? nullptr : it->second.get();
}

void SceneManager::Update(float deltaTime)
{
    if (m_activeScene) m_activeScene->Update(deltaTime);
}

void SceneManager::FixedUpdate(float deltaTime)
{
    if (m_activeScene) m_activeScene->FixedUpdate(deltaTime);
}

RenderList SceneManager::BuildRenderList()
{
    if (!m_activeScene) return {};
    return m_activeScene->BuildRenderList();
}