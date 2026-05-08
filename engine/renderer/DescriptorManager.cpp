#include "DescriptorManager.h"
#include <utils/Logger.h>

void DescriptorManager::Init(Device& device, CommandManager& cmdManager)
{
    CreateLayout(device);
    CreatePool(device);
    CreateUBOs(device);
    CreateSets(device);
    Logger::Info("DescriptorManager initialized");
}

void DescriptorManager::Shutdown()
{
    //vk::raii handles destruction of sets, pool, layout, and buffers
    Logger::Info("DescriptorManager shutdown");
}

void DescriptorManager::CreateLayout(Device& device)
{
    //binding 0 — FrameData UBO, visible to vertex (and fragment for lighting later)
    vk::DescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = vk::ShaderStageFlagBits::eVertex |
        vk::ShaderStageFlagBits::eFragment;

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboBinding;

    m_layout = vk::raii::DescriptorSetLayout(device.GetDevice(), layoutInfo);
    Logger::Info("Descriptor set layout created");
}

void DescriptorManager::CreatePool(Device& device)
{
    vk::DescriptorPoolSize poolSize{};
    poolSize.type = vk::DescriptorType::eUniformBuffer;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;


    m_pool = vk::raii::DescriptorPool(device.GetDevice(), poolInfo);
    Logger::Info("Descriptor pool created");
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

void DescriptorManager::CreateSets(Device& device)
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_layout);

    vk::DescriptorSetAllocateInfo allocInfo{};
    allocInfo.descriptorPool = *m_pool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    m_sets = vk::raii::DescriptorSets(device.GetDevice(), allocInfo);

    //point each set at its UBO
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo bufInfo{};
        bufInfo.buffer = *m_ubos[i].GetBuffer();
        bufInfo.offset = 0;
        bufInfo.range = sizeof(FrameData);

        vk::WriteDescriptorSet write{};
        write.dstSet = m_sets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = vk::DescriptorType::eUniformBuffer;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufInfo;

        device.GetDevice().updateDescriptorSets(write, nullptr);
    }
    Logger::Info("Descriptor sets created and bound to UBOs");
}

void DescriptorManager::UpdateFrameData(uint32_t frameIndex, const FrameData& data)
{
    void* mapped = m_ubos[frameIndex].Map();
    memcpy(mapped, &data, sizeof(FrameData));
    m_ubos[frameIndex].Unmap();
}