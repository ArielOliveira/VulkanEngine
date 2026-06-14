#include <graphicsPipeline.hpp>

GraphicsPipeline::GraphicsPipeline(const Device &device, const Extent2D &swapChainExtent, const SurfaceFormatKHR &swapChainSurfaceFormat, vk::Format depthFormat) {
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

    vk::PipelineViewportStateCreateInfo viewportState {
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

    vk::PipelineDepthStencilStateCreateInfo depthStencil {
        .depthTestEnable        = vk::True,
        .depthWriteEnable       = vk::True,
        .depthCompareOp         = vk::CompareOp::eLess,
        .depthBoundsTestEnable  = vk::False,
        .stencilTestEnable      = vk::False
    };

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings {{
        { .binding          = 0,
          .descriptorType   = vk::DescriptorType::eUniformBuffer,
          .descriptorCount  = 1,
          .stageFlags       = vk::ShaderStageFlagBits::eVertex },
        
        { .binding          = 1,
          .descriptorType   = vk::DescriptorType::eCombinedImageSampler,
          .descriptorCount  = 1,
          .stageFlags       = vk::ShaderStageFlagBits::eFragment }
    }};

    vk::DescriptorSetLayoutCreateInfo layoutInfo {
        .bindingCount     = static_cast<uint32_t>(bindings.size()),
        .pBindings        = bindings.data()
    };

    descriptorSetLayout = DescriptorSetLayout(device, layoutInfo);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
        .setLayoutCount         = 1,
        .pSetLayouts            = &*descriptorSetLayout,
        .pushConstantRangeCount = 0
    };

    pipelineLayout = PipelineLayout(device, pipelineLayoutInfo);

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
        {.stageCount            = 2,
         .pStages               = shaderStages,
         .pVertexInputState     = &vertexInputInfo,
         .pInputAssemblyState   = &inputAssembly,
         .pViewportState        = &viewportState,
         .pRasterizationState   = &rasterizer,
         .pMultisampleState     = &multisampling,
         .pDepthStencilState    = &depthStencil,
         .pColorBlendState      = &colorBlending,
         .pDynamicState         = &dynamicState,
         .layout                = pipelineLayout,
         .renderPass            = nullptr},
        
        {.colorAttachmentCount    = 1,
         .pColorAttachmentFormats = &swapChainSurfaceFormat.format,
         .depthAttachmentFormat   = depthFormat}
    }; 

    pipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());


}

GraphicsPipeline::GraphicsPipeline(std::nullptr_t) noexcept {}
GraphicsPipeline::~GraphicsPipeline() {}

GraphicsPipeline& GraphicsPipeline::operator=(std::nullptr_t) noexcept {
    descriptorSets.clear();

    descriptorPool.clear();
    descriptorPool = nullptr;

    descriptorSetLayout.clear();
    descriptorSetLayout = nullptr;

    pipelineLayout.clear();
    pipelineLayout = nullptr;
    
    pipeline.clear();
    pipeline = nullptr;

    return *this;
}

const vk::raii::Pipeline& GraphicsPipeline::getInstance() const & { return pipeline; }
const PipelineLayout&      GraphicsPipeline::getPipelineLayout() const & { return pipelineLayout; }
const DescriptorSetLayout& GraphicsPipeline::getDescriptorSetLayout() const & { return descriptorSetLayout; }
const DescriptorSet& GraphicsPipeline::getDescriptorSet(uint32_t frameIndex) const {
    if (frameIndex < 0 || frameIndex > descriptorSets.size()-1)
        throw std::runtime_error("out of bounds!");

        return descriptorSets[frameIndex];
}

void GraphicsPipeline::createDescriptorPool(const Device &device) {
    std::array<vk::DescriptorPoolSize, 2> poolSize {{
        { .type            = vk::DescriptorType::eUniformBuffer,
          .descriptorCount = MAX_FRAMES_IN_FLIGHT },
        { .type            = vk::DescriptorType::eCombinedImageSampler,
          .descriptorCount = MAX_FRAMES_IN_FLIGHT}
    }};

    vk::DescriptorPoolCreateInfo poolInfo {
        .flags           = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets         = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount   = static_cast<uint32_t>(poolSize.size()),
        .pPoolSizes      = poolSize.data()
    };

    descriptorPool = DescriptorPool(device, poolInfo);
}

void GraphicsPipeline::createDescriptorSets(const Device &device, const vector<vk::raii::Buffer> &uniformBuffers, const vk::DescriptorImageInfo &imageInfo) {
    vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo {
        .descriptorPool         = descriptorPool,
        .descriptorSetCount     = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts            = layouts.data()
    };

    descriptorSets = device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo {
            .buffer = uniformBuffers[i],
            .offset = 0,
            .range  = sizeof(UniformBufferObject)
        };

        std::array<vk::WriteDescriptorSet, 2> descriptorWrites {{
            { .dstSet                 = descriptorSets[i],
              .dstBinding             = 0,
              .dstArrayElement        = 0,
              .descriptorCount        = 1,
              .descriptorType         = vk::DescriptorType::eUniformBuffer,
              .pBufferInfo            = &bufferInfo },
            
            { .dstSet                 = descriptorSets[i],
              .dstBinding             = 1,
              .dstArrayElement        = 0,
              .descriptorCount        = 1,
              .descriptorType         = vk::DescriptorType::eCombinedImageSampler,
              .pImageInfo             = &imageInfo }
        }};

        device.updateDescriptorSets(descriptorWrites, {});
    }
}
