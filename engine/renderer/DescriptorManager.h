#pragma once
#include <renderer/VulkanIncludes.h>
#include <renderer/Device.h>
#include <renderer/Buffer.h>
#include <renderer/FrameData.h>
#include <renderer/CommandManager.h>

// Owns descriptor infrastructure for two descriptor set layouts:
//
//   set 0 — per-frame UBO (FrameData: view/proj/lights)
//            one UBO + one DescriptorSet per frame in flight
//
//   set 1 — per-material textures (diffuse/specular/gloss/normal)
//            layout shared across all materials
//            pool shared — Material::Init allocates individual sets from it

class DescriptorManager
{
public:
    void Init(Device& device, CommandManager& cmdManager);
    void Shutdown();

    // Write view/proj/light data into the per-frame UBO
    void UpdateFrameData(uint32_t frameIndex, const FrameData& data);

    // set 0
    vk::DescriptorSetLayout      GetFrameLayout()    const { return *m_frameLayout; }
    vk::raii::DescriptorSet& GetFrameSet(uint32_t frameIndex) { return m_frameSets[frameIndex]; }

    // set 1 — layout and pool handed to Material::Init
    vk::DescriptorSetLayout      GetMaterialLayout() const { return *m_materialLayout; }
    vk::DescriptorPool           GetMaterialPool()   const { return *m_materialPool; }

private:
    // set 0 — per-frame
    vk::raii::DescriptorSetLayout        m_frameLayout = nullptr;
    vk::raii::DescriptorPool             m_framePool = nullptr;
    std::vector<vk::raii::DescriptorSet> m_frameSets;
    std::vector<Buffer>                  m_ubos;

    // set 1 — per-material (layout + shared pool; sets owned by Material)
    vk::raii::DescriptorSetLayout        m_materialLayout = nullptr;
    vk::raii::DescriptorPool             m_materialPool = nullptr;

    void CreateFrameLayout(Device& device);
    void CreateFramePool(Device& device);
    void CreateUBOs(Device& device);
    void CreateFrameSets(Device& device);

    void CreateMaterialLayout(Device& device);
    void CreateMaterialPool(Device& device);

    // Legacy name kept so callers that haven't updated yet still compile
    vk::DescriptorSetLayout GetLayout() const { return GetFrameLayout(); }
};