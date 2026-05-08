#pragma once
#include <renderer/VulkanIncludes.h>
#include <renderer/Device.h>
#include <vector>
#include <string>
#include <filesystem>

struct PipelineConfig
{
    //shader paths(.spv)
    std::filesystem::path vertShaderPath;
    std::filesystem::path fragShaderPath;

    //dynamic rendering attachs
    std::vector<vk::Format> colorAttachmentFormats;
    vk::Format              depthAttachmentFormat = vk::Format::eUndefined;
    vk::Format              stencilAttachmentFormat = vk::Format::eUndefined;

    //rasterization
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
    vk::CullModeFlags       cullMode = vk::CullModeFlagBits::eBack; //no culling
    vk::FrontFace           frontFace = vk::FrontFace::eCounterClockwise;
    bool                    depthTest = true;
    bool                    depthWrite = true;
    bool                    blendEnable = false;

    //push constants
    uint32_t pushConstantSize = 0;
    vk::ShaderStageFlags pushConstantStages = vk::ShaderStageFlagBits::eVertex;

    //descriptorset layouts
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;

    bool useVertexInput = false;  //false = hardcoded shader vertices, true = bound VBO
    vk::VertexInputBindingDescription                    vertexBinding{};
    std::vector<vk::VertexInputAttributeDescription>     vertexAttributes{};
};

class Pipeline
{
public:
    void Init(Device& device, const PipelineConfig& config);
    void Shutdown();

    vk::raii::Pipeline& GetPipeline() { return m_pipeline; }
    vk::raii::PipelineLayout& GetPipelineLayout() { return m_pipelineLayout; }

    void Bind(vk::raii::CommandBuffer& cmd) const;

private:
    vk::raii::PipelineLayout m_pipelineLayout = nullptr;
    vk::raii::Pipeline       m_pipeline = nullptr;

    vk::raii::ShaderModule CreateShaderModule(Device& device,
        const std::filesystem::path& path);
};