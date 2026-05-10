#include "DescriptorManager.h"
#include <utils/Logger.h>

//maximum number of materials that can exist simultaneously.
//ach material allocates 4 combined-image-sampler descriptors from m_materialPool.
static constexpr uint32_t MAX_MATERIALS = 256;

void DescriptorManager::Init(Device& device, CommandManager& cmdManager)
{
    // set 0 — per-frame UBO
    CreateFrameLayout(device);
    CreateFramePool(device);
    CreateUBOs(device);
    CreateFrameSets(device);

    // set 1 — per-material textures
    CreateMaterialLayout(device);
    CreateMaterialPool(device);

    Logger::Info("DescriptorManager initialized");
}

void DescriptorManager::Shutdown()
{
    // vk::raii handles destruction of sets, pools, layouts, and buffers
    Logger::Info("DescriptorManager shutdown");
}

// ── set 0 ────────────────────────────────────────────────────────────────────

void DescriptorManager::CreateFrameLayout(Device& device)
{
    // binding 0 — FrameData UBO, visible to vertex and fragment stages
    vk::DescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = vk::ShaderStageFlagBits::eVertex |
        vk::ShaderStageFlagBits::eFragment;

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboBinding;

    m_frameLayout = vk::raii::DescriptorSetLayout(device.GetDevice(), layoutInfo);
    Logger::Info("Frame descriptor set layout created (set 0)");
}

void DescriptorManager::CreateFramePool(Device& device)
{
    vk::DescriptorPoolSize poolSize{};
    poolSize.type = vk::DescriptorType::eUniformBuffer;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    m_framePool = vk::raii::DescriptorPool(device.GetDevice(), poolInfo);
    Logger::Info("Frame descriptor pool created");
}

void DescriptorManager::CreateUBOs(Device& device)
{
    m_ubos.resize(MAX_FRAMES_IN_FLIGHT);
    for (auto& ubo : m_ubos)
    {
        ubo.Init(device,
            sizeof(FrameData),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
    }
    Logger::Info("UBO buffers created: ", MAX_FRAMES_IN_FLIGHT);
}

void DescriptorManager::CreateFrameSets(Device& device)
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_frameLayout);

    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.descriptorPool = *m_framePool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    m_frameSets = vk::raii::DescriptorSets(device.GetDevice(), allocInfo);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo bufInfo{};
        bufInfo.buffer = *m_ubos[i].GetBuffer();
        bufInfo.offset = 0;
        bufInfo.range = sizeof(FrameData);

        vk::WriteDescriptorSet write{};
        write.dstSet = m_frameSets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = vk::DescriptorType::eUniformBuffer;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufInfo;

        device.GetDevice().updateDescriptorSets(write, nullptr);
    }
    Logger::Info("Frame descriptor sets created and bound to UBOs");
}

void DescriptorManager::UpdateFrameData(uint32_t frameIndex, const FrameData& data)
{
    void* mapped = m_ubos[frameIndex].Map();
    memcpy(mapped, &data, sizeof(FrameData));
    m_ubos[frameIndex].Unmap();
}

// ── set 1 ─────────────────────────────────────────────────────────────────────

void DescriptorManager::CreateMaterialLayout(Device& device)
{
    // 4 combined-image-sampler bindings — one per texture slot
    // binding 0 diffuse | 1 specular | 2 gloss | 3 normal
    std::array<vk::DescriptorSetLayoutBinding, 4> bindings{};
    for (uint32_t i = 0; i < 4; i++)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = vk::ShaderStageFlagBits::eFragment;
    }

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    m_materialLayout = vk::raii::DescriptorSetLayout(device.GetDevice(), layoutInfo);
    Logger::Info("Material descriptor set layout created (set 1)");
}

void DescriptorManager::CreateMaterialPool(Device& device)
{
    // 4 samplers per material × MAX_MATERIALS materials
    vk::DescriptorPoolSize poolSize{};
    poolSize.type = vk::DescriptorType::eCombinedImageSampler;
    poolSize.descriptorCount = 4 * MAX_MATERIALS;

    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_MATERIALS;

    m_materialPool = vk::raii::DescriptorPool(device.GetDevice(), poolInfo);
    Logger::Info("Material descriptor pool created (max ", MAX_MATERIALS, " materials)");
}