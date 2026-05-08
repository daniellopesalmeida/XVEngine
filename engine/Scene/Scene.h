#pragma once
#include <scene/Camera.h>
#include <scene/SceneObject.h>
#include <scene/ObjectHandle.h>
#include <scene/RenderList.h>
#include <renderer/Device.h>
#include <renderer/CommandManager.h>
#include <renderer/Mesh.h>
#include <renderer/Vertex.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <core/Transform.h>

class Scene
{
public:
    void Init(Device& device, CommandManager& cmdManager);
    void Shutdown();

    //add an object from raw geometry
    ObjectHandle AddObject(
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const Transform& transform = {},
        const std::string& name = "");

    //add an object reusing an already-added mesh by name
    ObjectHandle AddObject(const std::string& meshName,const Transform& transform = {});

    //load a mesh from an .obj file and register it under 'name'
    //use AddObject(name, transform) to place instances
    void LoadMesh(const std::filesystem::path& path, const std::string& name);

    void RemoveObject(ObjectHandle handle);

    //object access
    SceneObject& GetObject(ObjectHandle handle);
    const SceneObject& GetObject(ObjectHandle handle) const;
    bool HasObject(ObjectHandle handle) const;

    //camera
    Camera& GetCamera() { return m_camera; }

    //called by engine each frame
    void Update(float deltaTime);
    void FixedUpdate(float deltaTime);

    RenderList BuildRenderList() const;

private:
    Device* m_device = nullptr;
    CommandManager* m_cmdManager = nullptr;

    Camera m_camera;

    std::vector<SceneObject> m_objects;
    std::vector<bool> m_alive;   //slot alive flags
    std::vector<std::unique_ptr<Mesh>> m_meshes;  //scene owns meshes

    //name tomesh ptr 
    std::unordered_map<std::string, Mesh*> m_meshRegistry;

    uint32_t m_nextId = 0;

    Mesh* GetOrCreateMesh(
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        const std::string& name);
};