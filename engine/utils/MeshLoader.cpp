#define TINYOBJLOADER_IMPLEMENTATION
#include "MeshLoader.h"
#include <tiny_obj_loader.h>
#include <utils/Logger.h>
#include <stdexcept>
#include <unordered_map>
#include <glm/glm.hpp>

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

// Compute per-vertex tangents using the standard UV-edge method.
// For each triangle (i0,i1,i2) we solve the 2x2 system:
//   [edge1, edge2] = [deltaUV1, deltaUV2] * [T, B]
// and accumulate the tangent T into each vertex.
// After all triangles are processed, each tangent is orthogonalised
// against its vertex normal via Gram-Schmidt and normalised.
static void ComputeTangents(std::vector<Vertex>& verts,
    const std::vector<uint32_t>& indices)
{
    // Accumulate raw tangents (un-normalised, summed across shared triangles)
    std::vector<glm::vec3> accum(verts.size(), glm::vec3(0.f));

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        const uint32_t i0 = indices[i + 0];
        const uint32_t i1 = indices[i + 1];
        const uint32_t i2 = indices[i + 2];

        const glm::vec3& p0 = verts[i0].position;
        const glm::vec3& p1 = verts[i1].position;
        const glm::vec3& p2 = verts[i2].position;

        const glm::vec2& uv0 = verts[i0].texCoord;
        const glm::vec2& uv1 = verts[i1].texCoord;
        const glm::vec2& uv2 = verts[i2].texCoord;

        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;

        glm::vec2 dUV1 = uv1 - uv0;
        glm::vec2 dUV2 = uv2 - uv0;

        float denom = dUV1.x * dUV2.y - dUV2.x * dUV1.y;
        if (std::abs(denom) < 1e-8f) continue;  // degenerate UV triangle

        float inv = 1.f / denom;
        glm::vec3 tangent = inv * (dUV2.y * edge1 - dUV1.y * edge2);

        accum[i0] += tangent;
        accum[i1] += tangent;
        accum[i2] += tangent;
    }

    // Gram-Schmidt orthogonalise each tangent against its vertex normal
    for (size_t i = 0; i < verts.size(); i++)
    {
        const glm::vec3& n = verts[i].normal;
        const glm::vec3& t = accum[i];

        if (glm::length(t) < 1e-8f)
        {
            // Vertex has no UV coverage — generate an arbitrary perpendicular
            glm::vec3 up = std::abs(n.y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
            verts[i].tangent = glm::normalize(glm::cross(up, n));
            continue;
        }

        // T' = normalize(T - dot(T,N)*N)
        verts[i].tangent = glm::normalize(t - glm::dot(t, n) * n);
    }
}

MeshLoader::MeshData MeshLoader::Load(const std::filesystem::path& path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string basedir = path.parent_path().string() + "/";

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials,
        &warn, &err,
        path.string().c_str(),
        basedir.c_str());

    if (!warn.empty()) Logger::Warn("MeshLoader: ", warn);
    if (!ok)
        throw std::runtime_error("MeshLoader failed to load: " +
            path.string() + "\n" + err);

    MeshData result;
    std::unordered_map<tinyobj::index_t, uint32_t, ObjIndexHash, ObjIndexEq> uniqueVerts;

    for (const auto& shape : shapes)
    {
        for (const auto& idx : shape.mesh.indices)
        {
            auto it = uniqueVerts.find(idx);
            if (it != uniqueVerts.end())
            {
                result.indices.push_back(it->second);
                continue;
            }

            Vertex v{};

            v.position = {
                attrib.vertices[3 * idx.vertex_index + 0],
                attrib.vertices[3 * idx.vertex_index + 1],
                attrib.vertices[3 * idx.vertex_index + 2]
            };

            if (idx.normal_index >= 0)
            {
                v.normal = {
                    attrib.normals[3 * idx.normal_index + 0],
                    attrib.normals[3 * idx.normal_index + 1],
                    attrib.normals[3 * idx.normal_index + 2]
                };
            }

            if (idx.texcoord_index >= 0)
            {
                v.texCoord = {
                    attrib.texcoords[2 * idx.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]  // Vulkan V-flip
                };
            }

            v.color = { 1.f, 1.f, 1.f };   // default white; material diffuse takes over
            v.tangent = { 0.f, 0.f, 0.f };   // filled in by ComputeTangents below

            if (result.vertices.size() >= UINT32_MAX)
                Logger::Warn("MeshLoader: mesh exceeds uint32_t index limit: ", path.string());

            auto newIndex = static_cast<uint32_t>(result.vertices.size());
            uniqueVerts[idx] = newIndex;
            result.vertices.push_back(v);
            result.indices.push_back(newIndex);
        }
    }

    ComputeTangents(result.vertices, result.indices);

    Logger::Info("Mesh loaded: '", path.filename().string(), "' — ",
        result.vertices.size(), " verts, ", result.indices.size(), " indices");

    return result;
}