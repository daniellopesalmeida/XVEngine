#include "Scene.h"
#include <utils/MeshLoader.h>
#include <utils/Logger.h>
#include <stdexcept>
#include <utils/MeshLoader.h>

void Scene::Init(Device& device, CommandManager& cmdManager)
{
    m_device = &device;
    m_cmdManager = &cmdManager;

    float aspect = 16.f / 9.f;  // updated when swapchain is known
    m_camera.Init(60.f, aspect, 0.01f, 1000.f);

    Logger::Info("Scene initialized");
}

void Scene::Shutdown()
{
    m_objects.clear();
    m_alive.clear();
    for (auto& mesh : m_meshes) mesh->Shutdown();
    m_meshes.clear();
    m_meshRegistry.clear();
    Logger::Info("Scene shutdown");
}

ObjectHandle Scene::AddObject(
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    const Transform& transform,
    const std::string& name)
{
    Mesh* mesh = GetOrCreateMesh(vertices, indices, name);

    SceneObject obj;
    obj.transform = transform;
    obj.mesh = mesh;
    obj.name = name;

    ObjectHandle handle{ m_nextId++ };
    m_objects.push_back(obj);
    m_alive.push_back(true);

    Logger::Info("Object added: '", name.empty() ? "unnamed" : name, "'");
    return handle;
}

ObjectHandle Scene::AddObject(const std::string& meshName,const Transform& transform)
{
    auto it = m_meshRegistry.find(meshName);
    if (it == m_meshRegistry.end())
    {
        throw std::runtime_error("Mesh not found in scene: " + meshName);
    }

    SceneObject obj;
    obj.transform = transform;
    obj.mesh = it->second;
    obj.name = meshName;

    ObjectHandle handle{ m_nextId++ };
    m_objects.push_back(obj);
    m_alive.push_back(true);

    Logger::Info("Object added (reused mesh): '", meshName, "'");
    return handle;
}

void Scene::RemoveObject(ObjectHandle handle)
{
    if (!HasObject(handle)) 
        return;

    m_alive[handle.id] = false;
    Logger::Info("Object removed: ", handle.id);
}

SceneObject& Scene::GetObject(ObjectHandle handle)
{
    if (!HasObject(handle))
    {
        throw std::runtime_error("Invalid object handle");
    }
    return m_objects[handle.id];
}

const SceneObject& Scene::GetObject(ObjectHandle handle) const
{
    if (!HasObject(handle))
    {
        throw std::runtime_error("Invalid object handle");
    }
    return m_objects[handle.id];
}

bool Scene::HasObject(ObjectHandle handle) const
{
    return handle.IsValid() && handle.id < m_alive.size() && m_alive[handle.id];
}

void Scene::Update(float deltaTime)
{
    m_camera.Update(deltaTime);
}

void Scene::FixedUpdate(float deltaTime)
{
    //physics / collision
}

RenderList Scene::BuildRenderList() const
{
    RenderList list;
    list.viewProj = m_camera.GetViewProj();

    for (uint32_t i = 0; i < m_objects.size(); i++)
    {
        if (!m_alive[i]) continue;
        const auto& obj = m_objects[i];
        if (!obj.mesh)   continue;

        DrawCall dc;
        dc.transform = obj.transform.GetMatrix();
        dc.mesh = obj.mesh;
        list.Add(dc);
    }

    return list;
}

void Scene::LoadMesh(const std::filesystem::path& path, const std::string& name)
{
    if (m_meshRegistry.count(name))
    {
        Logger::Warn("Scene::LoadMesh — mesh already registered: '", name, "', skipping.");
        return;
    }

    auto data = MeshLoader::Load(path);
    GetOrCreateMesh(data.vertices, data.indices, name);
}

Mesh* Scene::GetOrCreateMesh(const std::vector<Vertex>& vertices,const std::vector<uint32_t>& indices,const std::string& name)
{
    //reuse if named and already exists
    if (!name.empty())
    {
        auto it = m_meshRegistry.find(name);
        if (it != m_meshRegistry.end())
        {
            return it->second;
        }
    }

    auto mesh = std::make_unique<Mesh>();
    mesh->Init(*m_device, *m_cmdManager, vertices, indices);
    Mesh* ptr = mesh.get();
    m_meshes.push_back(std::move(mesh));

    if (!name.empty())
    {
        m_meshRegistry[name] = ptr;
    }

    return ptr;
}