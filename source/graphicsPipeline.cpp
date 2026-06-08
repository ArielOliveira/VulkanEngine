#include <graphicsPipeline.hpp>

GraphicsPipeline::GraphicsPipeline(const Device &device, const Extent2D &swapChainExtent, const SurfaceFormatKHR &swapChainSurfaceFormat) {
    // Create Shader Module
    string shaderAbsolutePath = FileHelper::getExecutablePath().generic_string();
    shaderAbsolutePath.append(shaderRelativePath);
    shaderAbsolutePath.append("helloTriangle.spv");

    auto shaderCode = FileHelper::readFile(shaderAbsolutePath, ios::ate | ios::binary);

    std::cout << "Shader size in bytes: " << shaderCode.size() << '\n';

    vk::ShaderModuleCreateInfo createInfo {
        .codeSize = shaderCode.size() * sizeof(char), 
        .pCode    = reinterpret_cast<const uint32_t*>(shaderCode.data())
    };

    ShaderModule shaderModule { device, createInfo };

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo {
        .stage               = vk::ShaderStageFlagBits::eVertex,
        .module              = shaderModule,
        .pName               = "vertMain",
        .pSpecializationInfo = nullptr // This is used for shader configuration through constants
    };

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo {
        .stage               = vk::ShaderStageFlagBits::eFragment,
        .module              = shaderModule,
        .pName               = "fragMain",
        .pSpecializationInfo = nullptr // This is used for shader configuration through constants
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] { vertShaderStageInfo, fragShaderStageInfo };
    vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates    = dynamicStates.data()
    };

    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions    = attributeDescriptions.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {
        .topology = vk::PrimitiveTopology::eTriangleList
    };

    vk::Viewport viewport {
        0.0f,
        0.0f,
        static_cast<float>(swapChainExtent.width),
        static_cast<float>(swapChainExtent.height),
        0.0f,
        1.0f
    };

    vk::Rect2D scissor {
        vk::Offset2D{0,0},
        swapChainExtent
    };

    vk::PipelineViewportStateCreateInfo viewPortState {
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount  = 1,
        .pScissors     = &scissor
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer {
        .depthClampEnable           = vk::False,
        .rasterizerDiscardEnable    = vk::False,
        .polygonMode                = vk::PolygonMode::eFill,
        .cullMode                   = vk::CullModeFlagBits::eBack,
        .frontFace                  = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable            = vk::False,
        .lineWidth                  = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable  = vk::False
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable    = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending {
        .logicOpEnable = vk::False, 
        .logicOp = vk::LogicOp::eCopy, 
        .attachmentCount = 1, 
        .pAttachments = &colorBlendAttachment
    };

    vk::DescriptorSetLayoutBinding uboLayoutBinding {
        .binding          = 0,
        .descriptorType   = vk::DescriptorType::eUniformBuffer,
        .descriptorCount  = 1,
        .stageFlags       = vk::ShaderStageFlagBits::eVertex
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo {
        .bindingCount     = 1,
        .pBindings        = &uboLayoutBinding
    };

    descriptorSetLayout = DescriptorSetLayout(device, layoutInfo);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
        .setLayoutCount         = 1,
        .pSetLayouts            = &*descriptorSetLayout,
        .pushConstantRangeCount = 0
    };

    pipelineLayout = PipelineLayout(device, pipelineLayoutInfo);

    vk::PipelineRenderingCreateInfo pipelineRenderingInfo {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChainSurfaceFormat.format
    };

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
        {.stageCount            = 2,
         .pStages               = shaderStages,
         .pVertexInputState     = &vertexInputInfo,
         .pInputAssemblyState   = &inputAssembly,
         .pViewportState        = &viewPortState,
         .pRasterizationState   = &rasterizer,
         .pMultisampleState     = &multisampling,
         .pColorBlendState      = &colorBlending,
         .pDynamicState         = &dynamicState,
         .layout                = pipelineLayout,
         .renderPass            = nullptr},
        
        {.colorAttachmentCount    = 1,
         .pColorAttachmentFormats = &swapChainSurfaceFormat.format}
    };

    pipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

GraphicsPipeline::GraphicsPipeline(std::nullptr_t) noexcept {}
GraphicsPipeline::~GraphicsPipeline() {}

GraphicsPipeline& GraphicsPipeline::operator=(std::nullptr_t) noexcept {
    pipeline.clear();
    pipeline = nullptr;

    pipelineLayout.clear();
    pipelineLayout = nullptr;

    descriptorSetLayout.clear();
    descriptorSetLayout = nullptr;

    return *this;
}

const vk::raii::Pipeline& GraphicsPipeline::getInstance() const & { return pipeline; }
const PipelineLayout&      GraphicsPipeline::getPipelineLayout() const & { return pipelineLayout; }
const DescriptorSetLayout& GraphicsPipeline::getDescriptorSetLayout() const & { return descriptorSetLayout; }