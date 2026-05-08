#include "Mesh.h"
#include "utils/Logger.h"

void Mesh::Init(Device& device, CommandManager& cmdManager,const std::vector<Vertex>& vertices,
    const std::vector<uint16_t>& indices)
{
    m_indexCount = static_cast<uint32_t>(indices.size());

    m_vertexBuffer = Buffer::CreateWithData(
        device, cmdManager,
        vertices.data(), sizeof(Vertex) * vertices.size(),
        vk::BufferUsageFlagBits::eVertexBuffer);

    m_indexBuffer = Buffer::CreateWithData(
        device, cmdManager,
        indices.data(), sizeof(uint16_t) * indices.size(),
        vk::BufferUsageFlagBits::eIndexBuffer);

    Logger::Info("Mesh created: ", vertices.size(), " verts, ", m_indexCount, " indices");
}

void Mesh::Shutdown()
{
    m_vertexBuffer.Shutdown();
    m_indexBuffer.Shutdown();
    Logger::Info("Mesh shutdown");
}