#include "Image.h"
#include "utils/Logger.h"
#include <stdexcept>

void Image::Init(Device& device, CommandManager& cmdManager, const ImageConfig& config)
{
    m_format = config.format;
    m_aspectMask = config.aspectMask;
    m_extent.width = config.width;
    m_extent.height =  config.height;
    m_mipLevels = config.mipLevels;

    CreateImage(device, config);
    AllocateMemory(device);
    CreateImageView(device);

    Logger::Info("Image created: ", config.width, "x", config.height,
        " format=", vk::to_string(config.format));
}

void Image::Shutdown()
{
    //vk::raii handles destruction order: view-memory- image
    Logger::Info("Image shutdown");
}

void Image::CreateImage(Device& device, const ImageConfig& config)
{
    vk::ImageCreateInfo imageInfo;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.format = config.format;
    imageInfo.extent = vk::Extent3D{ config.width, config.height, 1 };
    imageInfo.mipLevels = config.mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = config.samples;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = config.usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;

    m_image = vk::raii::Image(device.GetDevice(), imageInfo);
}

void Image::AllocateMemory(Device& device)
{
    auto memReqs = m_image.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(device,
        memReqs.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    m_memory = vk::raii::DeviceMemory(device.GetDevice(), allocInfo);
    m_image.bindMemory(*m_memory, 0);
}

void Image::CreateImageView(Device& device)
{
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image = *m_image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = m_format;

    viewInfo.subresourceRange.aspectMask = m_aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    m_imageView = vk::raii::ImageView(device.GetDevice(), viewInfo);
}

void Image::TransitionLayout(Device& device, CommandManager& cmdManager,
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    auto cmd = cmdManager.BeginSingleTimeCommands(device);

    vk::ImageMemoryBarrier2 barrier;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.image = *m_image;

    barrier.subresourceRange.aspectMask = m_aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    //access masks and stages from layouts
    auto [srcStage, srcAccess, dstStage, dstAccess] = [&]()
        -> std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2,
        vk::PipelineStageFlags2, vk::AccessFlags2>
        {
            using PS = vk::PipelineStageFlagBits2;
            using AC = vk::AccessFlagBits2;

            if (oldLayout == vk::ImageLayout::eUndefined &&
                newLayout == vk::ImageLayout::eColorAttachmentOptimal)
                return { PS::eTopOfPipe,     AC::eNone,
                         PS::eColorAttachmentOutput, AC::eColorAttachmentWrite };

            if (oldLayout == vk::ImageLayout::eUndefined &&
                newLayout == vk::ImageLayout::eDepthAttachmentOptimal)
                return { PS::eTopOfPipe,     AC::eNone,
                         PS::eEarlyFragmentTests | PS::eLateFragmentTests,
                         AC::eDepthStencilAttachmentWrite };

            if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal &&
                newLayout == vk::ImageLayout::ePresentSrcKHR)
                return { PS::eColorAttachmentOutput, AC::eColorAttachmentWrite,
                         PS::eBottomOfPipe,           AC::eNone };

            if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal &&
                newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
                return { PS::eColorAttachmentOutput, AC::eColorAttachmentWrite,
                         PS::eFragmentShader,          AC::eShaderRead };

            // General fallback
            return { PS::eAllCommands, AC::eMemoryWrite,
                     PS::eAllCommands, AC::eMemoryRead | AC::eMemoryWrite };
        }();

    barrier.srcStageMask = srcStage;
    barrier.srcAccessMask = srcAccess;
    barrier.dstStageMask = dstStage;
    barrier.dstAccessMask = dstAccess;

    vk::DependencyInfo depInfo;
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;

    cmd->pipelineBarrier2(depInfo);
    cmdManager.EndSingleTimeCommands(device, *cmd);
}

uint32_t Image::FindMemoryType(Device& device,
    uint32_t typeFilter,
    vk::MemoryPropertyFlags properties)
{
    auto memProps = device.GetPhysicalDevice().getMemoryProperties();
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    throw std::runtime_error("Failed to find suitable memory type");
}