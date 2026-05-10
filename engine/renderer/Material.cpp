#include "Material.h"
#include <utils/Logger.h>
#include <stdexcept>
#include <array>

void Material::Init(Device& device, CommandManager& cmdManager,
    vk::DescriptorPool pool, vk::DescriptorSetLayout layout,
    const MaterialDesc& desc)
{
    // Diffuse is sRGB-encoded — mark it so Texture::Init picks eR8G8B8A8Srgb.
    // Data maps (specular, gloss, normal) are linear — leave isSrgb false.
    m_diffuse = LoadOrFallback(device, cmdManager, desc.diffuse, TextureLoader::White(), true);
    m_specular = LoadOrFallback(device, cmdManager, desc.specular, TextureLoader::Grey(), false);
    m_gloss = LoadOrFallback(device, cmdManager, desc.gloss, TextureLoader::Grey(), false);
    m_normal = LoadOrFallback(device, cmdManager, desc.normal, TextureLoader::FlatNormal(), false);

    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    auto sets = vk::raii::DescriptorSets(device.GetDevice(), allocInfo);
    m_set = std::move(sets.front());

    WriteDescriptors(device);

    Logger::Info("Material initialized");
}

void Material::Shutdown()
{
    m_normal.reset();
    m_gloss.reset();
    m_specular.reset();
    m_diffuse.reset();
    m_set = nullptr;
    Logger::Info("Material shutdown");
}

void Material::Bind(vk::raii::CommandBuffer& cmd,
    vk::PipelineLayout layout,
    uint32_t setIndex) const
{
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        layout,
        setIndex,
        *m_set,
        nullptr);
}

std::unique_ptr<Texture> Material::LoadOrFallback(
    Device& device, CommandManager& cmdManager,
    const std::filesystem::path& path,
    const TextureData& fallback,
    bool isSrgb)
{
    auto tex = std::make_unique<Texture>();
    if (!path.empty() && std::filesystem::exists(path))
    {
        TextureData data = TextureLoader::Load(path);
        data.isSrgb = isSrgb;   // set before Upload so Texture picks the right format
        tex->Init(device, cmdManager, data);
    }
    else
    {
        // Fallback textures are 1x1 — isSrgb doesn't matter visually for solid colours,
        // but we keep it consistent with what the slot expects.
        TextureData data = fallback;
        data.isSrgb = isSrgb;
        tex->Init(device, cmdManager, data);
    }
    return tex;
}

void Material::WriteDescriptors(Device& device)
{
    std::array<vk::DescriptorImageInfo, 4> imageInfos = {
        m_diffuse->GetDescriptorInfo(),
        m_specular->GetDescriptorInfo(),
        m_gloss->GetDescriptorInfo(),
        m_normal->GetDescriptorInfo(),
    };

    std::array<vk::WriteDescriptorSet, 4> writes{};
    for (uint32_t i = 0; i < 4; i++)
    {
        writes[i].dstSet = *m_set;
        writes[i].dstBinding = i;
        writes[i].dstArrayElement = 0;
        writes[i].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writes[i].descriptorCount = 1;
        writes[i].pImageInfo = &imageInfos[i];
    }

    device.GetDevice().updateDescriptorSets(writes, {});
}