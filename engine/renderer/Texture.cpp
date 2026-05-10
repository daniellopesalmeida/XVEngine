#include "Texture.h"
#include <renderer/Buffer.h>
#include <utils/Logger.h>
#include <stdexcept>
#include <cmath>

void Texture::Init(Device& device, CommandManager& cmdManager, const TextureData& data)
{
    if (data.pixels.empty())
        throw std::runtime_error("Texture::Init — empty pixel data");

    // HDR textures stay as float, no mips for now (environment maps handle their own)
    if (data.isHDR)
    {
        m_mipLevels = 1;

        ImageConfig cfg{};
        cfg.width = data.width;
        cfg.height = data.height;
        cfg.format = vk::Format::eR32G32B32A32Sfloat;
        cfg.usage = vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferDst;
        cfg.aspectMask = vk::ImageAspectFlagBits::eColor;
        cfg.samples = vk::SampleCountFlagBits::e1;
        cfg.mipLevels = m_mipLevels;

        m_image.Init(device, cmdManager, cfg);
        UploadPixels(device, cmdManager, data);
        CreateSampler(device);
        return;
    }

    // Compute full mip chain depth: floor(log2(max(w, h))) + 1
    m_mipLevels = static_cast<uint32_t>(
        std::floor(std::log2(std::max(data.width, data.height)))) + 1;

    // Diffuse/albedo textures are sRGB-encoded — GPU must linearise on sample.
    // Data maps (normal, specular, gloss) are already linear — use Unorm.
    vk::Format format = data.isSrgb
        ? vk::Format::eR8G8B8A8Srgb
        : vk::Format::eR8G8B8A8Unorm;

    ImageConfig cfg{};
    cfg.width = data.width;
    cfg.height = data.height;
    cfg.format = format;
    // eTransferSrc needed so each mip level can be used as blit source
    cfg.usage = vk::ImageUsageFlagBits::eSampled |
        vk::ImageUsageFlagBits::eTransferDst |
        vk::ImageUsageFlagBits::eTransferSrc;
    cfg.aspectMask = vk::ImageAspectFlagBits::eColor;
    cfg.samples = vk::SampleCountFlagBits::e1;
    cfg.mipLevels = m_mipLevels;

    m_image.Init(device, cmdManager, cfg);
    UploadPixels(device, cmdManager, data);   // uploads mip 0, leaves it in TransferDst
    GenerateMips(device, cmdManager);         // blits down the chain, ends ShaderReadOnly
    CreateSampler(device);
}

void Texture::Shutdown()
{
    m_sampler = nullptr;
    m_image.Shutdown();
}

Texture Texture::FromFile(Device& device, CommandManager& cmdManager,
    const std::filesystem::path& path)
{
    Texture t;
    t.Init(device, cmdManager, TextureLoader::Load(path));
    return t;
}

vk::DescriptorImageInfo Texture::GetDescriptorInfo() const
{
    vk::DescriptorImageInfo info{};
    info.sampler = *m_sampler;
    info.imageView = *m_image.GetImageView();
    info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    return info;
}

void Texture::UploadPixels(Device& device, CommandManager& cmdManager, const TextureData& data)
{
    vk::DeviceSize byteSize = static_cast<vk::DeviceSize>(data.pixels.size());

    Buffer staging = Buffer::CreateWithData(
        device, cmdManager,
        data.pixels.data(), byteSize,
        vk::BufferUsageFlagBits::eTransferSrc);

    // Transition the entire image (all mips) to TransferDst so we can copy into mip 0.
    // GenerateMips will handle transitioning individual levels afterwards.
    m_image.TransitionLayout(device, cmdManager,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal);

    auto cmd = cmdManager.BeginSingleTimeCommands(device);

    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = vk::Offset3D{ 0, 0, 0 };
    region.imageExtent = vk::Extent3D{ data.width, data.height, 1 };

    cmd->copyBufferToImage(
        *staging.GetBuffer(),
        *m_image.GetImage(),
        vk::ImageLayout::eTransferDstOptimal,
        region);

    cmdManager.EndSingleTimeCommands(device, *cmd);
    // Mip 0 is now in TransferDstOptimal — GenerateMips takes it from here.
}

void Texture::GenerateMips(Device& device, CommandManager& cmdManager)
{
    // Check the physical device supports blitting from this format.
    // (All common colour formats support it, but good practice to verify.)
    auto formatProps = device.GetPhysicalDevice().getFormatProperties(m_image.GetFormat());
    if (!(formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        throw std::runtime_error("Texture format does not support linear blit for mip generation");

    auto cmd = cmdManager.BeginSingleTimeCommands(device);

    int32_t mipW = static_cast<int32_t>(m_image.GetExtent().width);
    int32_t mipH = static_cast<int32_t>(m_image.GetExtent().height);

    for (uint32_t i = 1; i < m_mipLevels; i++)
    {
        // Transition level (i-1): TransferDst → TransferSrc so we can blit from it
        vk::ImageMemoryBarrier2 toSrc{};
        toSrc.image = *m_image.GetImage();
        toSrc.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        toSrc.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        toSrc.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
        toSrc.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
        toSrc.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
        toSrc.dstAccessMask = vk::AccessFlagBits2::eTransferRead;
        toSrc.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        toSrc.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        toSrc.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        toSrc.subresourceRange.baseMipLevel = i - 1;
        toSrc.subresourceRange.levelCount = 1;
        toSrc.subresourceRange.baseArrayLayer = 0;
        toSrc.subresourceRange.layerCount = 1;

        vk::DependencyInfo dep{};
        dep.imageMemoryBarrierCount = 1;
        dep.pImageMemoryBarriers = &toSrc;
        cmd->pipelineBarrier2(dep);

        // Blit level (i-1) → level (i) at half resolution
        int32_t nextW = mipW > 1 ? mipW / 2 : 1;
        int32_t nextH = mipH > 1 ? mipH / 2 : 1;

        vk::ImageBlit2 blit{};
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.srcOffsets[0] = vk::Offset3D{ 0,    0,    0 };
        blit.srcOffsets[1] = vk::Offset3D{ mipW, mipH, 1 };

        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D{ 0,     0,     0 };
        blit.dstOffsets[1] = vk::Offset3D{ nextW, nextH, 1 };

        vk::BlitImageInfo2 blitInfo{};
        blitInfo.srcImage = *m_image.GetImage();
        blitInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
        blitInfo.dstImage = *m_image.GetImage();
        blitInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
        blitInfo.regionCount = 1;
        blitInfo.pRegions = &blit;
        blitInfo.filter = vk::Filter::eLinear;

        cmd->blitImage2(blitInfo);

        // Transition level (i-1): TransferSrc → ShaderReadOnly — it's done
        vk::ImageMemoryBarrier2 toShader{};
        toShader.image = *m_image.GetImage();
        toShader.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        toShader.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        toShader.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
        toShader.srcAccessMask = vk::AccessFlagBits2::eTransferRead;
        toShader.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
        toShader.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        toShader.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        toShader.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
        toShader.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        toShader.subresourceRange.baseMipLevel = i - 1;
        toShader.subresourceRange.levelCount = 1;
        toShader.subresourceRange.baseArrayLayer = 0;
        toShader.subresourceRange.layerCount = 1;

        dep.pImageMemoryBarriers = &toShader;
        cmd->pipelineBarrier2(dep);

        mipW = nextW;
        mipH = nextH;
    }

    // Transition the last mip level: TransferDst → ShaderReadOnly
    vk::ImageMemoryBarrier2 lastMip{};
    lastMip.image = *m_image.GetImage();
    lastMip.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    lastMip.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    lastMip.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
    lastMip.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    lastMip.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
    lastMip.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
    lastMip.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    lastMip.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    lastMip.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    lastMip.subresourceRange.baseMipLevel = m_mipLevels - 1;
    lastMip.subresourceRange.levelCount = 1;
    lastMip.subresourceRange.baseArrayLayer = 0;
    lastMip.subresourceRange.layerCount = 1;

    vk::DependencyInfo dep{};
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &lastMip;
    cmd->pipelineBarrier2(dep);

    cmdManager.EndSingleTimeCommands(device, *cmd);

    Logger::Info("Mips generated: ", m_mipLevels, " levels");
}

void Texture::CreateSampler(Device& device)
{
    auto props = device.GetPhysicalDevice().getProperties();

    vk::SamplerCreateInfo info{};
    info.magFilter = vk::Filter::eLinear;
    info.minFilter = vk::Filter::eLinear;
    info.addressModeU = vk::SamplerAddressMode::eRepeat;
    info.addressModeV = vk::SamplerAddressMode::eRepeat;
    info.addressModeW = vk::SamplerAddressMode::eRepeat;
    info.anisotropyEnable = vk::True;
    info.maxAnisotropy = props.limits.maxSamplerAnisotropy;
    info.borderColor = vk::BorderColor::eIntOpaqueBlack;
    info.unnormalizedCoordinates = vk::False;
    info.compareEnable = vk::False;
    info.compareOp = vk::CompareOp::eAlways;
    info.mipmapMode = vk::SamplerMipmapMode::eLinear;
    info.mipLodBias = 0.f;
    info.minLod = 0.f;
    info.maxLod = static_cast<float>(m_mipLevels);   // full chain

    m_sampler = vk::raii::Sampler(device.GetDevice(), info);
}