#include "Buffer.h"
#include "utils/Logger.h"
#include <stdexcept>

void Buffer::Init(Device& device, vk::DeviceSize size,vk::BufferUsageFlags usage,vk::MemoryPropertyFlags memProps)
{
    m_size = size;

    vk::BufferCreateInfo bufInfo{};
    bufInfo.size = size;
    bufInfo.usage = usage;
    bufInfo.sharingMode = vk::SharingMode::eExclusive;
    m_buffer = vk::raii::Buffer(device.GetDevice(), bufInfo);

    auto memReqs = m_buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(device, memReqs.memoryTypeBits, memProps);
    m_memory = vk::raii::DeviceMemory(device.GetDevice(), allocInfo);

    m_buffer.bindMemory(*m_memory, 0);
}

void Buffer::Shutdown()
{
    //vk::raii handles cleanup
}

Buffer Buffer::CreateWithData(Device& device, CommandManager& cmdManager,const void* data, vk::DeviceSize size,
    vk::BufferUsageFlags usage)
{
    //staging buffer
    Buffer staging;
    staging.Init(device, size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent);

    void* mapped = staging.Map();
    memcpy(mapped, data, static_cast<size_t>(size));
    staging.Unmap();

    //destination
    Buffer dst;
    dst.Init(device, size,
        usage | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    //copy
    auto cmd = cmdManager.BeginSingleTimeCommands(device);
    vk::BufferCopy copyRegion{};
    copyRegion.size = size;
    cmd->copyBuffer(*staging.GetBuffer(), *dst.GetBuffer(), copyRegion);
    cmdManager.EndSingleTimeCommands(device, *cmd);

    Logger::Info("Buffer uploaded: ", size, " bytes");
    return dst;
}

void* Buffer::Map()
{
    m_mapped = true;
    return m_memory.mapMemory(0, m_size);
}

void Buffer::Unmap()
{
    if (m_mapped)
    {
        m_memory.unmapMemory();
        m_mapped = false;
    }
}

uint32_t Buffer::FindMemoryType(Device& device,uint32_t typeFilter,vk::MemoryPropertyFlags props)
{
    auto memProps = device.GetPhysicalDevice().getMemoryProperties();
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    throw std::runtime_error("Failed to find suitable memory type for buffer");
}