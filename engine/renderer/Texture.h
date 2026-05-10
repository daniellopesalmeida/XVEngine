#pragma once
#include <renderer/VulkanIncludes.h>
#include <renderer/Device.h>
#include <renderer/CommandManager.h>
#include <renderer/Image.h>
#include <utils/TextureLoader.h>

// GPU texture — Image (VkImage + VkImageView) + VkSampler
// Created from TextureData (CPU pixels), uploaded via staging buffer.
// If TextureData::isSrgb is true, uses eR8G8B8A8Srgb format so the GPU
// linearises on sample (correct for diffuse/albedo textures).
// Generates a full mip chain for every non-HDR texture.

class Texture
{
public:
    void Init(Device& device, CommandManager& cmdManager, const TextureData& data);
    void Shutdown();

    static Texture FromFile(Device& device, CommandManager& cmdManager,
        const std::filesystem::path& path);

    vk::DescriptorImageInfo GetDescriptorInfo() const;

private:
    Image m_image;
    vk::raii::Sampler m_sampler = nullptr;
    uint32_t m_mipLevels = 1;

    void UploadPixels(Device& device, CommandManager& cmdManager, const TextureData& data);
    void GenerateMips(Device& device, CommandManager& cmdManager);
    void CreateSampler(Device& device);
};