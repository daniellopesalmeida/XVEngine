#include "Pipeline.h"
#include "utils/Logger.h"
#include <stdexcept>
#include <fstream>

static std::vector<char> ReadSpvFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open shader: " + path.string());

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    return buffer;
}

vk::raii::ShaderModule Pipeline::CreateShaderModule(Device& device,
    const std::filesystem::path& path)
{
    auto code = ReadSpvFile(path);

    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    return vk::raii::ShaderModule(device.GetDevice(), createInfo);
}

void Pipeline::Init(Device& device, const PipelineConfig& config)
{
    // ── Shader modules ────────────────────────────────────────────────
    auto vertModule = CreateShaderModule(device, config.vertShaderPath);
    auto fragModule = CreateShaderModule(device, config.fragShaderPath);

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { {
        { {}, vk::ShaderStageFlagBits::eVertex,   *vertModule, "main" },
        { {}, vk::ShaderStageFlagBits::eFragment, *fragModule, "main" }
    } };

    // ── Vertex input (empty — driven by push constants / SSBOs) ───────
    vk::PipelineVertexInputStateCreateInfo vertexInput{};
    if (config.useVertexInput)
    {
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &config.vertexBinding;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(config.vertexAttributes.size());
        vertexInput.pVertexAttributeDescriptions = config.vertexAttributes.data();
    }
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

    // ── Viewport / scissor (dynamic) ──────────────────────────────────
    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // ── Rasterizer ────────────────────────────────────────────────────
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.cullMode = config.cullMode;
    rasterizer.frontFace = config.frontFace;
    rasterizer.lineWidth = 1.0f;

    // ── MSAA ──────────────────────────────────────────────────────────
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.rasterizationSamples = config.msaaSamples;

    // ── Depth / stencil ───────────────────────────────────────────────
    vk::PipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.depthTestEnable = config.depthTest;
    depthStencil.depthWriteEnable = config.depthWrite;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;

    // ── Colour blend (one attachment per colour target) ───────────────
    std::vector<vk::PipelineColorBlendAttachmentState> blendAttachments(
        config.colorAttachmentFormats.size());

    for (auto& att : blendAttachments)
    {
        att.colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        att.blendEnable = config.blendEnable;
        if (config.blendEnable)
        {
            att.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
            att.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
            att.colorBlendOp = vk::BlendOp::eAdd;
            att.srcAlphaBlendFactor = vk::BlendFactor::eOne;
            att.dstAlphaBlendFactor = vk::BlendFactor::eZero;
            att.alphaBlendOp = vk::BlendOp::eAdd;
        }
    }

    vk::PipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
    colorBlend.pAttachments = blendAttachments.data();

    // ── Dynamic state ─────────────────────────────────────────────────
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // ── Pipeline layout ───────────────────────────────────────────────
    vk::PipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.setLayoutCount = static_cast<uint32_t>(config.descriptorSetLayouts.size());
    layoutInfo.pSetLayouts = config.descriptorSetLayouts.empty()
        ? nullptr
        : config.descriptorSetLayouts.data();

    vk::PushConstantRange pushRange{};
    if (config.pushConstantSize > 0)
    {
        pushRange.stageFlags = config.pushConstantStages;
        pushRange.offset = 0;
        pushRange.size = config.pushConstantSize;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushRange;
    }

    m_pipelineLayout = vk::raii::PipelineLayout(device.GetDevice(), layoutInfo);

    // ── Dynamic rendering (no VkRenderPass needed) ────────────────────
    vk::PipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.colorAttachmentCount =
        static_cast<uint32_t>(config.colorAttachmentFormats.size());
    renderingInfo.pColorAttachmentFormats = config.colorAttachmentFormats.data();
    renderingInfo.depthAttachmentFormat = config.depthAttachmentFormat;
    renderingInfo.stencilAttachmentFormat = config.stencilAttachmentFormat;

    // ── Graphics pipeline ─────────────────────────────────────────────
    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.pNext = &renderingInfo;   // chains dynamic rendering
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = *m_pipelineLayout;
    pipelineInfo.renderPass = nullptr;          // dynamic rendering = no renderpass

    m_pipeline = vk::raii::Pipeline(device.GetDevice(), VK_NULL_HANDLE, pipelineInfo);

    if (*m_pipeline == VK_NULL_HANDLE)
        throw std::runtime_error("Pipeline creation returned null handle");

    // confirm working directory
    Logger::Info("Working dir: ", std::filesystem::current_path().string());
    Logger::Info("Pipeline created: vert=", config.vertShaderPath.filename().string(),
        " frag=", config.fragShaderPath.filename().string());
    Logger::Info("Pipeline created successfully");
}

void Pipeline::Shutdown()
{
    Logger::Info("Pipeline shutdown");
    //vk::raii handles cleanup
}

void Pipeline::Bind(vk::raii::CommandBuffer& cmd) const
{
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
}