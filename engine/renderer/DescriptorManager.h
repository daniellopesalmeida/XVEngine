#pragma once
#include <renderer/VulkanIncludes.h>
#include <renderer/Device.h>
#include <renderer/Buffer.h>
#include <renderer/FrameData.h>
#include <renderer/CommandManager.h>

//owns descriptor infrastructure for per-frame UBO data (set 0).
//one UBO + one descriptor set per frame in flight.
class DescriptorManager
{
public:
    void Init(Device& device, CommandManager& cmdManager);
    void Shutdown();

    //write view/proj into the UBO for this frame
    void UpdateFrameData(uint32_t frameIndex, const FrameData& data);

    vk::DescriptorSetLayout GetLayout() const { return *m_layout; }
    vk::raii::DescriptorSet& GetDescriptorSet(uint32_t frameIndex) { return m_sets[frameIndex]; }

private:
    vk::raii::DescriptorSetLayout m_layout = nullptr;
    vk::raii::DescriptorPool m_pool = nullptr;
    std::vector<vk::raii::DescriptorSet> m_sets;
    std::vector<Buffer> m_ubos;  //one per frame in flight

    void CreateLayout(Device& device);
    void CreatePool(Device& device);
    void CreateUBOs(Device& device);
    void CreateSets(Device& device);
};