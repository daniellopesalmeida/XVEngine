#pragma once
#include <renderer/VulkanIncludes.h>
#include <renderer/Device.h>
#include <renderer/CommandManager.h>
#include <renderer/Texture.h>
#include <filesystem>
#include <memory>

// Describes which texture files to load for each slot.
// Leave a path empty ("") to use a built-in 1x1 fallback:
//   diffuse  → white   (sRGB)
//   specular → grey    (linear)
//   gloss    → grey    (linear)
//   normal   → flat    (linear, 128,128,255)
struct MaterialDesc
{
    std::filesystem::path diffuse;
    std::filesystem::path specular;
    std::filesystem::path gloss;
    std::filesystem::path normal;
};

class Material
{
public:
    void Init(Device& device, CommandManager& cmdManager,
        vk::DescriptorPool pool, vk::DescriptorSetLayout layout,
        const MaterialDesc& desc);
    void Shutdown();

    void Bind(vk::raii::CommandBuffer& cmd,
        vk::PipelineLayout layout,
        uint32_t setIndex = 1) const;

    vk::raii::DescriptorSet& GetDescriptorSet() { return m_set; }

private:
    std::unique_ptr<Texture> m_diffuse;
    std::unique_ptr<Texture> m_specular;
    std::unique_ptr<Texture> m_gloss;
    std::unique_ptr<Texture> m_normal;

    vk::raii::DescriptorSet m_set = nullptr;

    // isSrgb controls whether Texture::Init uses eR8G8B8A8Srgb or Unorm
    static std::unique_ptr<Texture> LoadOrFallback(
        Device& device, CommandManager& cmdManager,
        const std::filesystem::path& path,
        const TextureData& fallback,
        bool isSrgb);

    void WriteDescriptors(Device& device);
};