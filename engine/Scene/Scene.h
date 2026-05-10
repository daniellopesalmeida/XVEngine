#pragma once
#include <scene/Camera.h>
#include <scene/SceneObject.h>
#include <scene/ObjectHandle.h>
#include <scene/RenderList.h>
#include <renderer/Device.h>
#include <renderer/CommandManager.h>
#include <renderer/Mesh.h>
#include <renderer/Vertex.h>
#include <renderer/Material.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <core/Transform.h>

class DescriptorManager;

class Scene
{
public:
    void Init(Device& device, CommandManager& cmdManager,
        DescriptorManager& descriptorManager, float aspect);
    void Shutdown();

    // Add an object from raw geometry (no material)
    ObjectHandle AddObject(
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const Transform& transform = {},
        const std::string& name = "");

    // Add an object reusing an already-registered mesh, optionally with a named material
    ObjectHandle AddObject(const std::string& meshName,
        const Transform& transform = {},
        const std::string& materialName = "");

    // Load a mesh from an .obj file and register it under 'name'
    void LoadMesh(const std::filesystem::path& path, const std::string& name);

    // Load a material from a MaterialDesc and register it under 'name'
    void LoadMaterial(const std::string& name, const MaterialDesc& desc);

    void RemoveObject(ObjectHandle handle);

    SceneObject& GetObject(ObjectHandle handle);
    const SceneObject& GetObject(ObjectHandle handle) const;
    bool               HasObject(ObjectHandle handle) const;

    // Look up a registered material by name — returns nullptr if not found.
    // Use with GetObject() to swap materials at runtime:
    //   scene->GetObject(handle).material = scene->GetMaterial("damagedMat");
    // Pass nullptr to fall back to the renderer's default white material.
    Material* GetMaterial(const std::string& name)
    {
        auto it = m_materialRegistry.find(name);
        return it == m_materialRegistry.end() ? nullptr : it->second;
    }

    Camera& GetCamera() { return m_camera; }

    // Lighting — configure once in App::Load
    glm::vec3 lightDir = glm::vec3(0.f, -10.0f, -0.5f);
    glm::vec3 lightColor = { 1.f, 1.f, 1.f };
    float ambientStrength = 0.15f;
    float specularStrength = 0.5f;
    float shininess = 32.f;

    void Update(float deltaTime);
    void FixedUpdate(float deltaTime);

    RenderList BuildRenderList() const;

private:
    Device* m_device = nullptr;
    CommandManager* m_cmdManager = nullptr;
    DescriptorManager* m_descMgr = nullptr;

    Camera m_camera;

    std::vector<SceneObject>           m_objects;
    std::vector<bool>                  m_alive;
    std::vector<std::unique_ptr<Mesh>> m_meshes;
    std::unordered_map<std::string, Mesh*> m_meshRegistry;

    std::vector<std::unique_ptr<Material>>     m_materials;
    std::unordered_map<std::string, Material*> m_materialRegistry;

    uint32_t m_nextId = 0;

    Mesh* GetOrCreateMesh(
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const std::string& name);
};