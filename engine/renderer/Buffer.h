#pragma once
#include <renderer/VulkanIncludes.h>
#include <renderer/Device.h>
#include <renderer/CommandManager.h>

class Buffer
{
public:
    void Init(Device& device, vk::DeviceSize size,vk::BufferUsageFlags usage,vk::MemoryPropertyFlags memProps);
    void Shutdown();

    //data via a staging buffer
    static Buffer CreateWithData(Device& device, CommandManager& cmdManager,
        const void* data, vk::DeviceSize size,
        vk::BufferUsageFlags usage);

    vk::raii::Buffer& GetBuffer() { return m_buffer; }
    vk::raii::DeviceMemory& GetMemory() { return m_memory; }
    vk::DeviceSize GetSize()const { return m_size; }

    void* Map();
    void Unmap();

private:
    vk::raii::Buffer m_buffer = nullptr;
    vk::raii::DeviceMemory m_memory = nullptr;
    vk::DeviceSize m_size = 0;
    bool  m_mapped = false;

    uint32_t FindMemoryType(Device& device,uint32_t typeFilter,vk::MemoryPropertyFlags props);
};