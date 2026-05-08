#pragma once
#include <renderer/Vertex.h>
#include <vector>
#include <string>
#include <filesystem>

//loads .obj files via tinyobjloader
//throws std::runtime_error on failure
//warns if size is bigger than uint16_t (65535 unique vertices)
class MeshLoader
{
public:
    struct MeshData
    {
        std::vector<Vertex>   vertices;
        std::vector<uint32_t> indices;
    };

    static MeshData Load(const std::filesystem::path& path);
};