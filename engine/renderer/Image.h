#pragma once
#include <renderer/VulkanIncludes.h>
#include <renderer/Device.h>
#include <renderer/CommandManager.h>

struct ImageConfig
{
    uint32_t                 width = 0;
    uint32_t                 height = 0;
    vk::Format               format = vk::Format::eUndefined;
    vk::ImageUsageFlags      usage = vk::ImageUsageFlagBits::eColorAttachment;
    vk::ImageAspectFlags     aspectMask = vk::ImageAspectFlagBits::eColor;
    vk::SampleCountFlagBits  samples = vk::SampleCountFlagBits::e1;
    uint32_t                 mipLevels = 1;
};

class Image
{
public:
    void Init(Device& device, CommandManager& cmdManager, const ImageConfig& config);
    void Shutdown();

    vk::raii::Image& GetImage() { return m_image; }
    vk::raii::ImageView& GetImageView() { return m_imageView; }
    vk::Format           GetFormat()    const { return m_format; }
    vk::Extent2D         GetExtent()    const { return m_extent; }

    //transition layout (single-time command)
    void TransitionLayout(Device& device, CommandManager& cmdManager,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

private:
    vk::raii::Image        m_image = nullptr;
    vk::raii::DeviceMemory m_memory = nullptr;
    vk::raii::ImageView    m_imageView = nullptr;
    vk::Format             m_format = vk::Format::eUndefined;
    vk::ImageAspectFlags   m_aspectMask = vk::ImageAspectFlagBits::eColor;
    vk::Extent2D           m_extent = {};
    uint32_t               m_mipLevels = 1;

    void CreateImage(Device& device, const ImageConfig& config);
    void AllocateMemory(Device& device);
    void CreateImageView(Device& device);

    uint32_t FindMemoryType(Device& device,
        uint32_t typeFilter,
        vk::MemoryPropertyFlags properties);
};