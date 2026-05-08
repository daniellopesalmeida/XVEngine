#pragma once
#include <renderer/Buffer.h>
#include <renderer/Vertex.h>
#include <vector>

class Mesh
{
public:
    void Init(Device& device, CommandManager& cmdManager,const std::vector<Vertex>& vertices,
        const std::vector<uint16_t>& indices);

    void Shutdown();

    Buffer& GetVertexBuffer() { return m_vertexBuffer; }
    Buffer& GetIndexBuffer() { return m_indexBuffer; }
    uint32_t GetIndexCount() const { return m_indexCount; }

private:
    Buffer m_vertexBuffer;
    Buffer m_indexBuffer;
    uint32_t m_indexCount = 0;
};