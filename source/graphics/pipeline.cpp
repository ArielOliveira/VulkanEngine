#include <graphics/pipeline.hpp>

namespace Graphics {
    Pipeline::Pipeline(const vk::raii::Device &device, const vk::GraphicsPipelineCreateInfo &createInfo, vk::raii::PipelineLayout& pipelineLayout, vk::raii::DescriptorSetLayout &descriptorSetLayout) :
        pipeline(vk::raii::Pipeline(device, nullptr, createInfo)),
        pipelineLayout(std::move(pipelineLayout)),
        descriptorSetLayout(std::move(descriptorSetLayout)) {}

    Pipeline::Pipeline(const vk::raii::Device &device, const vk::GraphicsPipelineCreateInfo &createInfo, vk::raii::PipelineLayout& pipelineLayout) :
        pipeline(vk::raii::Pipeline(device, nullptr, createInfo)),
        pipelineLayout(std::move(pipelineLayout)) {}

    Pipeline::Pipeline(const vk::raii::Device &device, const vk::ComputePipelineCreateInfo &createInfo, vk::raii::PipelineLayout& pipelineLayout, vk::raii::DescriptorSetLayout &descriptorSetLayout) : 
        pipeline(vk::raii::Pipeline(device, nullptr, createInfo)),
        pipelineLayout(std::move(pipelineLayout)),
        descriptorSetLayout(std::move(descriptorSetLayout)) {}

    Pipeline Pipeline::createGraphicsPipeline(const vk::raii::Device &device, const vk::Extent2D &swapChainExtent, const vk::SurfaceFormatKHR &swapChainSurfaceFormat, vk::Format depthFormat, vk::SampleCountFlagBits msaaSamples) {
        vk::raii::ShaderModule shaderModule = createShaderModule("helloTriangle.spv", device);

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

        auto bindingDescription    = Models::Vertex::getBindingDescription();
        auto attributeDescriptions = Models::Vertex::getAttributeDescriptions();
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
            .rasterizationSamples = msaaSamples,
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

        vk::raii::DescriptorSetLayout descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
            .setLayoutCount         = 0,
            .pSetLayouts            = {},
            .pushConstantRangeCount = 0
        };

        vk::raii::PipelineLayout pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

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
        
        return std::move(Pipeline(device, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>(), pipelineLayout, descriptorSetLayout));
    }

    Pipeline Pipeline::createGraphicsPipelinePointList(const vk::raii::Device &device, const vk::Extent2D &swapChain, const vk::SurfaceFormatKHR &swapChainSurfaceFormat, vk::Format depthFormat, vk::SampleCountFlagBits msaaSamples) {
        vk::raii::ShaderModule shaderModule = createShaderModule("helloParticles.spv", device);

		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{.stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain"};
		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{.stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"};
		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		auto bindingDescription    = Models::Particle::getBindingDescription();
		auto attributeDescriptions = Models::Particle::getAttributeDescriptions();

		vk::PipelineVertexInputStateCreateInfo   vertexInputInfo{.vertexBindingDescriptionCount = 1, .pVertexBindingDescriptions = &bindingDescription, .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()), .pVertexAttributeDescriptions = attributeDescriptions.data()};
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::ePointList, .primitiveRestartEnable = vk::False};
		vk::PipelineViewportStateCreateInfo      viewportState{.viewportCount = 1, .scissorCount = 1};
		vk::PipelineRasterizationStateCreateInfo rasterizer{
		    .depthClampEnable        = vk::False,
		    .rasterizerDiscardEnable = vk::False,
		    .polygonMode             = vk::PolygonMode::eFill,
		    .cullMode                = vk::CullModeFlagBits::eBack,
		    .frontFace               = vk::FrontFace::eCounterClockwise,
		    .depthBiasEnable         = vk::False,
		    .lineWidth               = 1.0f};
		
        vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};

		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
		    .blendEnable         = vk::True,
		    .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
		    .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
		    .colorBlendOp        = vk::BlendOp::eAdd,
		    .srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
		    .dstAlphaBlendFactor = vk::BlendFactor::eZero,
		    .alphaBlendOp        = vk::BlendOp::eAdd,
		    .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
		};

		vk::PipelineColorBlendStateCreateInfo colorBlending{.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment};

		std::vector dynamicStates = {
		    vk::DynamicState::eViewport,
		    vk::DynamicState::eScissor};
		vk::PipelineDynamicStateCreateInfo dynamicState{.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
		vk::raii::PipelineLayout pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
		    {.stageCount           = 2,
		     .pStages              = shaderStages,
		     .pVertexInputState    = &vertexInputInfo,
		     .pInputAssemblyState  = &inputAssembly,
		     .pViewportState       = &viewportState,
		     .pRasterizationState  = &rasterizer,
		     .pMultisampleState    = &multisampling,
		     .pColorBlendState     = &colorBlending,
		     .pDynamicState        = &dynamicState,
		     .layout               = pipelineLayout,
		     .renderPass           = nullptr},
		    
            {.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format}
        };

        return std::move(Pipeline(device, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>(), pipelineLayout));
    }

    Pipeline Pipeline::createComputePipeline(const vk::raii::Device &device) {
        vk::raii::ShaderModule shaderModule = createShaderModule("tutorialParticleSystem.spv", device);

        std::array layoutBindings{
		    vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		    vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		    vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr)
        };

		vk::DescriptorSetLayoutCreateInfo layoutInfo{ 
            .bindingCount = static_cast<uint32_t>(layoutBindings.size()), 
            .pBindings = layoutBindings.data()
        };

        vk::PipelineShaderStageCreateInfo computeShaderStageInfo {
            .stage  = vk::ShaderStageFlagBits::eCompute,
            .module = shaderModule,
            .pName  = "compMain"
        };

		vk::raii::DescriptorSetLayout computeDescriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
            .setLayoutCount = 1,
            .pSetLayouts = &*computeDescriptorSetLayout
        };

        vk::raii::PipelineLayout computePipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

        vk::ComputePipelineCreateInfo pipelineCreateInfo {
            .stage  = computeShaderStageInfo,
            .layout = computePipelineLayout
        };

        return std::move(Pipeline(device, pipelineCreateInfo, computePipelineLayout, computeDescriptorSetLayout));
    }

    Pipeline::Pipeline(std::nullptr_t) noexcept {}
    Pipeline::~Pipeline() {}

    vk::raii::ShaderModule Pipeline::createShaderModule(const char* name, const vk::raii::Device &device) {
        // Create Shader Module
        string shaderAbsolutePath = Runtime::FileHelper::getExecutablePath().generic_string();
        shaderAbsolutePath.append(Models::SHADER_RELATIVE_PATH);
        shaderAbsolutePath.append(name);

        auto shaderCode = Runtime::FileHelper::readFile(shaderAbsolutePath, ios::ate | ios::binary);

        std::cout << "Shader size in bytes: " << shaderCode.size() << '\n';

        vk::ShaderModuleCreateInfo createInfo {
            .codeSize = shaderCode.size() * sizeof(char), 
            .pCode    = reinterpret_cast<const uint32_t*>(shaderCode.data())
        };

        return std::move(vk::raii::ShaderModule(device, createInfo));
    }

    Pipeline& Pipeline::operator=(std::nullptr_t) noexcept {
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

    const vk::raii::Pipeline& Pipeline::getInstance() const & { return pipeline; }
    const vk::raii::PipelineLayout&      Pipeline::getPipelineLayout() const & { return pipelineLayout; }
    const vk::raii::DescriptorSetLayout& Pipeline::getDescriptorSetLayout() const & { return descriptorSetLayout; }
    const vk::raii::DescriptorSet& Pipeline::getDescriptorSet(uint32_t frameIndex) const {
        if (frameIndex < 0 || frameIndex > descriptorSets.size()-1)
            throw std::runtime_error("out of bounds!");

            return descriptorSets[frameIndex];
    }

    void Pipeline::createGraphicsDescriptorPool(const vk::raii::Device &device) {
        std::array poolSize {
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, Models::MAX_FRAMES_IN_FLIGHT),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, Models::MAX_FRAMES_IN_FLIGHT)
        };

        vk::DescriptorPoolCreateInfo poolInfo {
            .flags           = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets         = Models::MAX_FRAMES_IN_FLIGHT,
            .poolSizeCount   = static_cast<uint32_t>(poolSize.size()),
            .pPoolSizes      = poolSize.data()
        };

        descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
    }

    void Pipeline::createGraphicsDescriptorSets(const vk::raii::Device &device, const vector<vk::raii::Buffer> &uniformBuffers, const vk::DescriptorImageInfo &imageInfo) {
        vector<vk::DescriptorSetLayout> layouts(Models::MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo {
            .descriptorPool      = descriptorPool,
            .descriptorSetCount  = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts         = layouts.data()
        };

        descriptorSets = device.allocateDescriptorSets(allocInfo);

        std::cout << layouts.size() << '\n';
        std::cout << descriptorSets.size() << '\n';

        /*for (size_t i = 0; i < Models::MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo bufferInfo {
                .buffer = uniformBuffers[i],
                .offset = 0,
                .range  = sizeof(Models::UniformBufferObject)
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
        }*/
    }

    void Pipeline::createComputeDescriptorPool(const vk::raii::Device &device) {
        std::array poolSize {
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, Models::MAX_FRAMES_IN_FLIGHT),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, Models::MAX_FRAMES_IN_FLIGHT * 2)
        };

        vk::DescriptorPoolCreateInfo poolInfo {
            .flags           = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets         = Models::MAX_FRAMES_IN_FLIGHT,
            .poolSizeCount   = static_cast<uint32_t>(poolSize.size()),
            .pPoolSizes      = poolSize.data()
        };

        descriptorPool       = vk::raii::DescriptorPool(device, poolInfo);
    }

    void Pipeline::createComputeDescriptorSets(const vk::raii::Device &device, const vector<vk::raii::Buffer> &uniformBuffers, const vector<vk::raii::Buffer> &shaderStorageBuffers, const uint32_t particleCount) {
        vector<vk::DescriptorSetLayout> layouts (Models::MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo {
            .descriptorPool     = *descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts        = layouts.data()
        };

        descriptorSets = device.allocateDescriptorSets(allocInfo);

        for (size_t i = 0; i < Models::MAX_FRAMES_IN_FLIGHT; i++) {
            
            vk::DescriptorBufferInfo bufferInfo(uniformBuffers[i], 0, sizeof(Models::GlobalInputs));
            
            vk::DescriptorBufferInfo storageBufferInfoLastFrame(shaderStorageBuffers[(i-1) % Models::MAX_FRAMES_IN_FLIGHT], 0, sizeof(Models::Particle) * particleCount);
            vk::DescriptorBufferInfo storageBufferInfoCurrentFrame(shaderStorageBuffers[i], 0, sizeof(Models::Particle) * particleCount);
            
            std::array<vk::WriteDescriptorSet, 3> descriptorWrites {{
                { .dstSet          = descriptorSets[i], 
                  .dstBinding      = 0, 
                  .dstArrayElement = 0, 
                  .descriptorCount = 1, 
                  .descriptorType  = vk::DescriptorType::eUniformBuffer, 
                  .pBufferInfo     = &bufferInfo },
                
                { .dstSet          = descriptorSets[i], 
                  .dstBinding      = 1, 
                  .dstArrayElement = 0, 
                  .descriptorCount = 1, 
                  .descriptorType  = vk::DescriptorType::eStorageBuffer, 
                  .pBufferInfo     = &storageBufferInfoLastFrame },

                { .dstSet          = descriptorSets[i], 
                  .dstBinding      = 2, 
                  .dstArrayElement = 0, 
                  .descriptorCount = 1, 
                  .descriptorType  = vk::DescriptorType::eStorageBuffer, 
                  .pBufferInfo     = &storageBufferInfoCurrentFrame },
            }};

            device.updateDescriptorSets(descriptorWrites, {});
        }
    }
}