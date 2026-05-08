#define TINYOBJLOADER_IMPLEMENTATION
#include "MeshLoader.h"
#include <tiny_obj_loader.h>
#include <utils/Logger.h>
#include <stdexcept>
#include <unordered_map>

//imple hash for deduplicating OBJ index triples
struct ObjIndexHash
{
    size_t operator()(const tinyobj::index_t& i) const
    {
        size_t h = std::hash<int>{}(i.vertex_index);
        h ^= std::hash<int>{}(i.normal_index) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>{}(i.texcoord_index) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

struct ObjIndexEq
{
    bool operator()(const tinyobj::index_t& a, const tinyobj::index_t& b) const
    {
        return a.vertex_index == b.vertex_index &&
            a.normal_index == b.normal_index &&
            a.texcoord_index == b.texcoord_index;
    }
};

MeshLoader::MeshData MeshLoader::Load(const std::filesystem::path& path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    //basedir lets tinyobj resolve relative mtl paths
    std::string basedir = path.parent_path().string() + "/";

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials,
        &warn, &err,
        path.string().c_str(),
        basedir.c_str());

    if (!warn.empty()) 
    {
        Logger::Warn("MeshLoader: ", warn);
    }
    if (!ok)           
    {
        throw std::runtime_error("MeshLoader failed to load: " +
            path.string() + "\n" + err);
    }

    MeshData result;
    std::unordered_map<tinyobj::index_t, uint16_t, ObjIndexHash, ObjIndexEq> uniqueVertices;

    for (const auto& shape : shapes)
    {
        for (const auto& idx : shape.mesh.indices)
        {
            auto it = uniqueVertices.find(idx);
            if (it != uniqueVertices.end())
            {
                result.indices.push_back(it->second);
                continue;
            }

            Vertex v{};

            //Position (always present)
            v.position = 
            {
                attrib.vertices[3 * idx.vertex_index + 0],
                attrib.vertices[3 * idx.vertex_index + 1],
                attrib.vertices[3 * idx.vertex_index + 2]
            };

            //Normal (optional)
            if (idx.normal_index >= 0)
            {
                v.normal = 
                {
                    attrib.normals[3 * idx.normal_index + 0],
                    attrib.normals[3 * idx.normal_index + 1],
                    attrib.normals[3 * idx.normal_index + 2]
                };
            }

            //Tex coord (optional) — flip V for Vulkan (OBJ origin is bottom-left)
            if (idx.texcoord_index >= 0)
            {
                v.texCoord = 
                {
                    attrib.texcoords[2 * idx.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]
                };
            }

            //Default vertex color to white
            v.color = { 1.f, 1.f, 1.f };

            if (result.vertices.size() >= 65535)
            {
                Logger::Warn("MeshLoader: mesh exceeds uint16_t index limit (65535 unique vertices): " + path.string());
            }

            auto newIndex = static_cast<uint32_t>(result.vertices.size());
            uniqueVertices[idx] = newIndex;
            result.vertices.push_back(v);
            result.indices.push_back(newIndex);
        }
    }

    Logger::Info("Mesh loaded: '", path.filename().string(), "' — ",
        result.vertices.size(), " verts, ", result.indices.size(), " indices");

    return result;
}