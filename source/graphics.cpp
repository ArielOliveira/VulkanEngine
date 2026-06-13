#include <graphics.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>

Graphics& Graphics::getInstance() {
    static Graphics instance;

    return instance;
}

Graphics::Graphics() {
    cout << "Initializing graphics..." << endl;

    createInstance();
#if DEBUG_MODE 
    setupDebugMessenger(); 
#endif
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    
    swapChain        = SwapChain(surface, physicalDevice, device);
    graphicsPipeline = GraphicsPipeline(device, swapChain.getExtent(), swapChain.getSurfaceFormat(), findDepthFormat());

    createCommandPool();
    createDepthResources();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    createTextureImage();
    createTextureSampler();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

Graphics::~Graphics() {}

void Graphics::createInstance() {
    Application& app = Application::getInstance();

    const vk::ApplicationInfo appInfo {
        .pApplicationName   = app.getInstance().name(),
        .applicationVersion = VK_MAKE_VERSION(VULKAN_ENGINE_VERSION_MAJOR, VULKAN_ENGINE_VERSION_MINOR, VULKAN_ENGINE_VERSION_PATCH),
        .pEngineName        = app.getInstance().name(),
        .engineVersion      = VK_MAKE_VERSION(VULKAN_ENGINE_VERSION_MAJOR, VULKAN_ENGINE_VERSION_MINOR, VULKAN_ENGINE_VERSION_PATCH),
        .apiVersion         = vk::ApiVersion14
    };

    uint32_t extensionCount = 0;
    vector<const char*> requiredExtensions = app.getRequiredExtensions(&extensionCount);
    
    try {
        std::vector<char const*> requiredLayers;
        auto layerProperties = context.enumerateInstanceLayerProperties();
        std::vector<vk::ExtensionProperties> extensionProperties = context.enumerateInstanceExtensionProperties();
        
        // ENABLE DEBUG FEATURES
    #if DEBUG_MODE
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
        requiredExtensions.push_back(vk::EXTDebugUtilsExtensionName);
            
        // CHECK FOR VALIDATION LAYERS
        if (std::ranges::any_of(
            requiredLayers,
            [&layerProperties](auto const &requiredLayer) {
                return std::ranges::none_of(layerProperties,
                [requiredLayer](auto const &layerProperty) {
                    return strcmp(layerProperty.layerName, requiredLayer) == 0;
                });
            })) {
                throw std::runtime_error("One or more required layers are not supported!");
            }

        cout << "Available device extensions" << '\n';

        for (const auto& extension : extensionProperties)
            cout << '\t' << extension.extensionName << '\n';
    #endif

        auto unsupportedPropertyIt = std::ranges::find_if(requiredExtensions, [&extensionProperties](auto const& requiredExtension) {
            return std::ranges::none_of(
                extensionProperties, 
                [requiredExtension](auto const& extensionProperty)
                { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }
            ); 
        });

        if (unsupportedPropertyIt != requiredExtensions.end())
            throw std::runtime_error("Required Platform extension not supported: " + std::string(*unsupportedPropertyIt));

        vk::InstanceCreateInfo createInfo {
            .pApplicationInfo        = &appInfo,
            .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames     = requiredLayers.data(),
            .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data()
        };

        instance = Instance(context, createInfo);

    } catch (const vk::SystemError& err) {
        cerr << "Vulkan Error: " << err.what() << '\n';
    } catch (const std::exception&  err) {
        cerr << "Error: " << err.what() << '\n';
    }
}

const bool Graphics::isDeviceSuitable(PhysicalDevice const& physicalDevice) const {
    auto queueFamilies = physicalDevice.getQueueFamilyProperties();

    bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;
    bool supportsGraphicsAndPresentation = false;

    uint32_t queueIndex = ~0;

    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilies.size(); qfpIndex++) {
        if ((queueFamilies[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) {
                supportsGraphicsAndPresentation = true;
                queueIndex = qfpIndex;
                break;
            }
    }

    if (queueIndex == ~0) 
        throw std::runtime_error("Could not find a queue for graphics and presentation -> terminating");

    vector<const char*> requiredDeviceExtensions = {vk::KHRSwapchainExtensionName};

    auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();

    bool supportsAllRequiredExtensions = 
        std::ranges::all_of(
            requiredDeviceExtensions, 
            [&availableDeviceExtensions](auto const& requiredDeviceExtension) {
                return std::ranges::any_of(
                    availableDeviceExtensions,
                    [requiredDeviceExtension](auto const& availableDeviceExtension)
                    { return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0; });                    
        });
        
    auto features                 = physicalDevice.template getFeatures2<
        vk::PhysicalDeviceFeatures2, 
        vk::PhysicalDeviceVulkan11Features, 
        vk::PhysicalDeviceVulkan13Features, 
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    >();
        
    bool supportsRequiredFeatures = 
        features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
        features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
        features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
        features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

    return supportsVulkan1_3 && supportsGraphicsAndPresentation && supportsAllRequiredExtensions && supportsRequiredFeatures;
}

void Graphics::pickPhysicalDevice() {
    auto physicalDevices = instance.enumeratePhysicalDevices();

    auto const deviceIt = 
        std::ranges::find_if(
            physicalDevices, 
            [&](auto const& physicalDevice) { return isDeviceSuitable(physicalDevice); } 
        );

    if (deviceIt == physicalDevices.end())
        throw std::runtime_error("Failed to find a suitable GPU!");

    physicalDevice = *deviceIt;
}

#if DEBUG_MODE
void Graphics::setupDebugMessenger() {
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );

    vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral     | 
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | 
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    
     vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT {
        .messageSeverity = severityFlags,
        .messageType     = messageTypeFlags,
        .pfnUserCallback = &debugCallback
    };

    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}
#endif

void Graphics::createLogicalDevice() {
    vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
    
    bool foundGraphicsAndPresentation = false;
    bool foundDedicatedTransfer       = false;
    
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) {
            graphicsQueueIndex = qfpIndex;

            foundGraphicsAndPresentation = true;
        } else if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eTransfer) &&
                  !(queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
                  !(queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eCompute)) {
            transferQueueIndex = qfpIndex;

            cout << "Found dedicated transfer queue." << '\n';

            foundDedicatedTransfer = true;    
        }

        if (foundDedicatedTransfer && foundGraphicsAndPresentation) break;
    }

    if (graphicsQueueIndex == ~0)
        throw std::runtime_error("Could not find a queue for graphics and presentation when creating logical device -> terminating");
    
    float graphicsQueuePriority = 0.5f;
    float transferQueuePriority = 0.5f;  

    vk::PhysicalDeviceFeatures deviceFeatures;

    vk::StructureChain<
            vk::PhysicalDeviceFeatures2, 
            vk::PhysicalDeviceVulkan11Features, 
            vk::PhysicalDeviceVulkan13Features, 
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        { .features = { .samplerAnisotropy = true }},
        { .shaderDrawParameters = true }, 
        { .synchronization2 = true,
          .dynamicRendering = true},
        { .extendedDynamicState = true }
    };

    vector<const char*> requiredDeviceExtensions = {
        vk::KHRSwapchainExtensionName
    };

    uint32_t queueCount = foundDedicatedTransfer ? 2 : 1;
    std::array<vk::DeviceQueueCreateInfo, 2> queueCreateInfos;

    vk::DeviceQueueCreateInfo deviceGraphicsQueueCreateInfo { 
        .queueFamilyIndex = graphicsQueueIndex,
        .queueCount       = 1,
        .pQueuePriorities = &graphicsQueuePriority
    };

    if (foundDedicatedTransfer) {
        vk::DeviceQueueCreateInfo deviceTransferQueueCreateInfo {
            .queueFamilyIndex = transferQueueIndex,
            .queueCount       = 1,
            .pQueuePriorities = &transferQueuePriority
        };

        queueCreateInfos = { deviceGraphicsQueueCreateInfo, deviceTransferQueueCreateInfo };
    }

    vk::DeviceCreateInfo deviceCreateInfo {
        .pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount    = queueCount,
        .pQueueCreateInfos       = foundDedicatedTransfer ? queueCreateInfos.data() : &deviceGraphicsQueueCreateInfo,
        .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = requiredDeviceExtensions.data()
    };

    device            = Device(physicalDevice, deviceCreateInfo);
    graphicsQueue     = Queue(device, graphicsQueueIndex, 0);
    if (foundDedicatedTransfer)
        transferQueue = Queue(device, transferQueueIndex, 0);
}

void Graphics::createSurface() {
    VkSurfaceKHR _surface;

    if (glfwCreateWindowSurface(*instance, Application::getInstance().getWindow(), nullptr, &_surface) != 0) 
        throw std::runtime_error("failed to create window surface");

    surface = SurfaceKHR(instance, _surface);
}

void Graphics::createCommandPool() {
    vk::CommandPoolCreateInfo graphicsPoolInfo {
        .flags              = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex   = graphicsQueueIndex
    };

    graphicsCommandPool = CommandPool(device, graphicsPoolInfo);

    // Review flags usage. Tutorial suggests using only eTransient, while AI chatbot (shame on me)
    // mentions short-lived command buffers (i.e transfers) will get reset often (?) so eResetCommandBuffer 
    // is also appropriate.
    vk::CommandPoolCreateInfo transferPoolInfo {
        .flags              = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex   = transferQueueIndex
    };

    transferCommandPool = CommandPool(device, transferPoolInfo);
}

void Graphics::createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocInfo {
        .commandPool        = graphicsCommandPool,
        .level              = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void Graphics::createSyncObjects() {
    assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());

    for (size_t i = 0; i < swapChain.getImageCount(); i++)
        renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        presentCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
        inFlightFences.emplace_back(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

void Graphics::createDepthResources() {
    vk::Format depthFormat = findDepthFormat();

    std::tie(depthImage, depthImageMemory) = createImage(swapChain.getExtent().width, swapChain.getExtent().height, 1, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
    depthImageView                         = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
}

void Graphics::loadModel() {
    tinyobj::attrib_t attrib;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;
    string                      err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, MODEL_PATH.c_str()))
        throw std::runtime_error(err);


    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0 - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            auto [it, inserted] = uniqueVertices.insert({vertex, static_cast<uint32_t>(vertices.size())});

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (inserted)  vertices.push_back(vertex);
            indices.push_back(it->second);
        }

        cout << vertices.size() << " vertices loaded." << '\n';
    }
}

void Graphics::createVertexBuffer() {
    std::array<uint32_t, 2> queueFamilyIndices = { graphicsQueueIndex, transferQueueIndex };
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    auto [stagingBuffer, stagingBufferMemory]  = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    std::tie(vertexBuffer, vertexBufferMemory) = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer       | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        transferQueueIndex == ~0 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
        transferQueueIndex == ~0 ? 0                           : 2,
        transferQueueIndex == ~0 ? nullptr                     : queueFamilyIndices.data()
    );

    void* data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
}

void Graphics::createIndexBuffer() {
    std::array<uint32_t, 2> queueFamilyIndices = { graphicsQueueIndex, transferQueueIndex };
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    auto [stagingBuffer, stagingBufferMemory]  = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    void *data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, indices.data(), (size_t)bufferSize);
    stagingBufferMemory.unmapMemory();

    std::tie(indexBuffer, indexBufferMemory) = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eIndexBuffer        | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        transferQueueIndex == ~0 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
        transferQueueIndex == ~0 ? 0                           : 2,
        transferQueueIndex == ~0 ? nullptr                     : queueFamilyIndices.data()
    );

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

void Graphics::createTextureImage() {
    int texWidth, texHeight, texChannels;

    stbi_uc *pixels             = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize    = texWidth * texHeight * 4;

    if (!pixels) 
        throw std::runtime_error("failed to load texture image!");

    auto [stagingBuffer, stagingBufferMemory] = createBuffer(
        imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // Copy pixels into GPU memory
    void* data = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(data, pixels, imageSize);
    stagingBufferMemory.unmapMemory();

    stbi_image_free(pixels);
    textureMipCount = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    std::tie(textureImage, textureImageMemory) = createImage(
        texWidth,
        texHeight,
        textureMipCount,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    CommandBuffer commandBuffer = beginSingleTimeCommand(transferQueueIndex == ~0 ? graphicsCommandPool : transferCommandPool);
    transitionImageLayout(commandBuffer,                          *textureImage, 
                          {},                                     vk::ImageLayout::eTransferDstOptimal,
                          {},                                     vk::AccessFlagBits2::eTransferWrite,
                          vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eTransfer,
                          vk::ImageAspectFlagBits::eColor,
                          textureMipCount);
    
    copyBufferToImage(commandBuffer, stagingBuffer, *textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    // Queue ownership release from transferQueue->graphicsQueue.
    transitionImageLayout(commandBuffer,                         *textureImage,
                          vk::ImageLayout::eTransferDstOptimal,  vk::ImageLayout::eTransferSrcOptimal,
                          vk::AccessFlagBits2::eTransferWrite,   {},
                          vk::PipelineStageFlagBits2::eTransfer, {},
                          vk::ImageAspectFlagBits::eColor,
                          textureMipCount,
                          transferQueueIndex == ~0 ? vk::QueueFamilyIgnored : transferQueueIndex,
                          transferQueueIndex == ~0 ? vk::QueueFamilyIgnored : graphicsQueueIndex);
    
    endSingleTimeCommand(std::move(commandBuffer), transferQueueIndex == ~0 ? graphicsQueue : transferQueue);
    
    // If we used a dedicated transfer queue we need to acquire ownership in the graphics queue.
    if (transferQueueIndex != ~0) { 
        CommandBuffer graphicsCommand = beginSingleTimeCommand(graphicsCommandPool);
        
        // Queue ownership acquisition from trasnferQueue->graphicsQueue
        transitionImageLayout(graphicsCommand, *textureImage,
                              {},              vk::ImageLayout::eTransferDstOptimal,
                              {},              vk::AccessFlagBits2::eTransferRead,
                              {},              vk::PipelineStageFlagBits2::eTransfer,
                              vk::ImageAspectFlagBits::eColor,
                              textureMipCount,
                              transferQueueIndex == ~0 ? vk::QueueFamilyIgnored : transferQueueIndex,
                              transferQueueIndex == ~0 ? vk::QueueFamilyIgnored : graphicsQueueIndex);
        endSingleTimeCommand(std::move(graphicsCommand), graphicsQueue);
    }

    // Mipmap generation involves a blit call which needs to happen in a graphics queue
    generateMipmaps(
        *textureImage, 
        texWidth, texHeight,
        textureMipCount,
        vk::Format::eR8G8B8A8Srgb, 
        vk::ImageTiling::eOptimal, 
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, 
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    textureImageView = createImageView(*textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, textureMipCount);
}

void Graphics::generateMipmaps(const vk::Image &image, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels,
                               vk::Format imageFormat, vk::ImageTiling tiling, vk::ImageUsageFlags usageFlags, vk::MemoryPropertyFlagBits memoryType) {
    // Check if image format supports linear blit-ing
    vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(imageFormat);

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) 
        throw std::runtime_error("texture image format does not support linear blitting!");
    
    CommandBuffer commandBuffer = beginSingleTimeCommand(graphicsCommandPool);

    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask           = vk::PipelineStageFlagBits2::eTransfer, .srcAccessMask          = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask           = vk::PipelineStageFlagBits2::eTransfer, .dstAccessMask          = vk::AccessFlagBits2::eTransferRead,
        .oldLayout              = vk::ImageLayout::eTransferDstOptimal,  .newLayout              = vk::ImageLayout::eTransferSrcOptimal,
        .srcQueueFamilyIndex    = vk::QueueFamilyIgnored,                .dstQueueFamilyIndex    = vk::QueueFamilyIgnored,
        .image                  = image,
        .subresourceRange       = {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };

    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags         = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier
    };

    int32_t mipWidth  = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout       = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout       = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask   = vk::AccessFlagBits2::eTransferWrite;
        barrier.dstAccessMask   = vk::AccessFlagBits2::eTransferRead;
        barrier.dstStageMask    = vk::PipelineStageFlagBits2::eTransfer;

        commandBuffer.pipelineBarrier2(dependencyInfo);

        vk::ArrayWrapper1D<vk::Offset3D, 2> srcOffsets {{{
            {0, 0, 0}, {mipWidth, mipHeight, 1}
        }}}; 
        
        vk::ArrayWrapper1D<vk::Offset3D, 2> dstOffsets {{{
            {0, 0, 0}, {mipWidth > 1 ? (mipWidth >> 1) : 1, mipHeight > 1 ? (mipHeight >> 1) : 1, 1}
        }}};
        
        vk::ImageBlit2 imageBlit = { 
            .srcSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i - 1, .baseArrayLayer = 0, .layerCount = 1 }, 
            .srcOffsets = srcOffsets,
            .dstSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i, .baseArrayLayer = 0, .layerCount = 1 }, 
            .dstOffsets = dstOffsets
        };

        vk::BlitImageInfo2 blitInfo {
            .srcImage    = image, .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
            .dstImage    = image, .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
            .pRegions    = &imageBlit,
            .filter      = vk::Filter::eLinear
        };
        
        commandBuffer.blitImage2(blitInfo);

        barrier.oldLayout       = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout       = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask   = vk::AccessFlagBits2::eTransferRead;
        barrier.dstAccessMask   = vk::AccessFlagBits2::eShaderRead;
        barrier.dstStageMask    = vk::PipelineStageFlagBits2::eFragmentShader;

        commandBuffer.pipelineBarrier2(dependencyInfo);

        if (mipWidth  > 1) mipWidth  >>= 1;
        if (mipHeight > 1) mipHeight >>= 1;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout       = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout       = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask   = vk::AccessFlagBits2::eTransferWrite;
    barrier.dstAccessMask   = vk::AccessFlagBits2::eShaderRead;
    barrier.dstStageMask    = vk::PipelineStageFlagBits2::eFragmentShader;

    commandBuffer.pipelineBarrier2(dependencyInfo);

    endSingleTimeCommand(std::move(commandBuffer), graphicsQueue);
}

void Graphics::createTextureSampler() {
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

    vk::SamplerCreateInfo samplerInfo {
        .magFilter                  = vk::Filter::eLinear,
        .minFilter                  = vk::Filter::eLinear,
        .mipmapMode                 = vk::SamplerMipmapMode::eLinear,
        .addressModeU               = vk::SamplerAddressMode::eRepeat,
        .addressModeV               = vk::SamplerAddressMode::eRepeat,
        .addressModeW               = vk::SamplerAddressMode::eRepeat,
        .mipLodBias                 = 0.0f,
        .anisotropyEnable           = vk::True,
        .maxAnisotropy              = properties.limits.maxSamplerAnisotropy,
        .compareEnable              = vk::False,
        .compareOp                  = vk::CompareOp::eAlways,
        .minLod                     = static_cast<float>(textureMipCount / 2),
        .maxLod                     = vk::LodClampNone,
        .borderColor                = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates    = vk::False
    };

    textureSampler = vk::raii::Sampler(device, samplerInfo);
}

vk::raii::ImageView Graphics::createImageView(const vk::Image &image, vk::Format format, vk::ImageAspectFlagBits aspectMaskFlags, uint32_t mipLevels) {
    vk::ImageViewCreateInfo viewInfo {
    .image            = image,
    .viewType         = vk::ImageViewType::e2D,
    .format           = format,
    .subresourceRange = {.aspectMask = aspectMaskFlags, .baseMipLevel = 0, .levelCount = mipLevels, .baseArrayLayer = 0, .layerCount = 1}};

    return vk::raii::ImageView(device, viewInfo);
}

void Graphics::createUniformBuffers() {
    std::array<uint32_t, 2> queueFamilyIndices = { graphicsQueueIndex, transferQueueIndex };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        auto [buffer, bufferMem] = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            transferQueueIndex == ~0 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
            transferQueueIndex == ~0 ? 0                           : 2,
            transferQueueIndex == ~0 ? nullptr                     : queueFamilyIndices.data()
        );

        uniformBuffers.emplace_back(std::move(buffer));
        uniformBuffersMemory.emplace_back(std::move(bufferMem));
        uniformBuffersMapped.emplace_back(uniformBuffersMemory.back().mapMemory(0, bufferSize));
    }
}

void Graphics::createDescriptorPool() {
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

void Graphics::createDescriptorSets() {
    vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *graphicsPipeline.getDescriptorSetLayout());
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

        vk::DescriptorImageInfo imageInfo {
            .sampler     = textureSampler,
            .imageView   = textureImageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
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

vk::Format Graphics::findDepthFormat() {
    return findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
         vk::ImageTiling::eOptimal,
         vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

vk::Format Graphics::findSupportedFormat(const vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (const auto format : candidates) {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);

        if (((tiling == vk::ImageTiling::eLinear) && ((props.linearTilingFeatures & features) == features)) ||
            ((tiling == vk::ImageTiling::eOptimal) && ((props.optimalTilingFeatures & features) == features))) {
                return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

std::pair<Buffer, DeviceMemory> Graphics::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::SharingMode sharingMode, uint32_t queueCount, const uint32_t* queueIndices) {
    vk::BufferCreateInfo bufferInfo {
        .size                   = size,
        .usage                  = usage,
        .sharingMode            = sharingMode,
        .queueFamilyIndexCount  = queueCount,
        .pQueueFamilyIndices    = queueIndices
    };

    Buffer buffer = Buffer(device, bufferInfo);
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memAllocateInfo = { 
        .allocationSize = memRequirements.size, 
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties) 
    };

    DeviceMemory bufferMemory = DeviceMemory(device, memAllocateInfo);
    buffer.bindMemory(*bufferMemory, 0);

    return { std::move(buffer), std::move(bufferMemory) };
}

std::pair<vk::raii::Image, DeviceMemory> Graphics::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::SharingMode sharingMode, uint32_t queueCount, const uint32_t* queueIndices) {
    vk::ImageCreateInfo imageInfo {
        .imageType         = vk::ImageType::e2D,
        .format            = format,
        .extent            = {width, height, 1},
        .mipLevels         = mipLevels,
        .arrayLayers       = 1,
        .samples           = vk::SampleCountFlagBits::e1,
        .tiling            = tiling,
        .usage             = usage,
        .sharingMode       = sharingMode
    };

    vk::raii::Image image = vk::raii::Image(device, imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo {
        .allocationSize   = memRequirements.size,
        .memoryTypeIndex  = findMemoryType(memRequirements.memoryTypeBits, properties)
    };

    DeviceMemory imageMemory = DeviceMemory(device, allocInfo);
    image.bindMemory(imageMemory, 0);

    return { std::move(image), std::move(imageMemory) };
}

void Graphics::copyBuffer(Buffer &srcBuffer, Buffer &dstBuffer, vk::DeviceSize size) {
    CommandBuffer copyCommandBuffer = beginSingleTimeCommand(transferCommandPool);
    copyCommandBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy(0, 0, size));
    endSingleTimeCommand(std::move(copyCommandBuffer), transferQueue);
}

void Graphics::copyBufferToImage(CommandBuffer &commandBuffer, const Buffer &buffer, const vk::Image &image, uint32_t width, uint32_t height) {
    vk::BufferImageCopy2 region {
        .bufferOffset       = 0,
        .bufferRowLength    = 0,
        .imageSubresource   = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
        .imageOffset        = { 0, 0, 0 },
        .imageExtent        = { width, height, 1}
    };

    vk::CopyBufferToImageInfo2 copyInfo {
        .srcBuffer       = buffer,
        .dstImage        = image,
        .dstImageLayout  = vk::ImageLayout::eTransferDstOptimal,
        .regionCount     = 1,
        .pRegions        = &region
    };

    commandBuffer.copyBufferToImage2(copyInfo);
}

CommandBuffer Graphics::beginSingleTimeCommand(const CommandPool &commandPool) {
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};
    vk::raii::CommandBuffer       commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    commandBuffer.begin(beginInfo);

    return std::move(commandBuffer);
}

void Graphics::endSingleTimeCommand(CommandBuffer &&commandBuffer, const Queue &queue) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();
}

void Graphics::recordCommandBuffer(uint32_t imageIndex) {
    commandBuffers[frameIndex].begin({});

    transitionImageLayout(
        commandBuffers[frameIndex],
        swapChain.getImage(imageIndex),
        ImageLayout::eUndefined,
        ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor,
        1
    );

    transitionImageLayout(
        commandBuffers[frameIndex],
        depthImage,
        ImageLayout::eUndefined,
        ImageLayout::eDepthAttachmentOptimal,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::ImageAspectFlagBits::eDepth,
        1
    );

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0.0f);

    vk::RenderingAttachmentInfo colorAttachmentInfo {
        .imageView      = swapChain.getImageView(imageIndex),
        .imageLayout    = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp         = vk::AttachmentLoadOp::eClear,
        .storeOp        = vk::AttachmentStoreOp::eStore,
        .clearValue     = clearColor
    };

    vk::RenderingAttachmentInfo depthAttachmentInfo {
        .imageView      = depthImageView,
        .imageLayout    = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp         = vk::AttachmentLoadOp::eClear,
        .storeOp        = vk::AttachmentStoreOp::eDontCare,
        .clearValue     = clearDepth
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea           = {.offset = {0, 0}, .extent = swapChain.getExtent()},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &colorAttachmentInfo,
        .pDepthAttachment     = &depthAttachmentInfo
    };

    commandBuffers[frameIndex].beginRendering(renderingInfo);
    commandBuffers[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.getInstance());
    commandBuffers[frameIndex].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.getExtent().width), static_cast<float>(swapChain.getExtent().height), 0.0f, 1.0f));
    commandBuffers[frameIndex].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.getExtent()));
    commandBuffers[frameIndex].bindVertexBuffers(0, *vertexBuffer, {0});
    commandBuffers[frameIndex].bindIndexBuffer(*indexBuffer, 0, vk::IndexTypeValue<decltype(indices)::value_type>::value);
    commandBuffers[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipeline.getPipelineLayout(), 0, *descriptorSets[frameIndex], nullptr);
    commandBuffers[frameIndex].drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    commandBuffers[frameIndex].endRendering();

    transitionImageLayout(
        commandBuffers[frameIndex],
        swapChain.getImage(imageIndex),
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::ImageAspectFlagBits::eColor,
        1
    );

    commandBuffers[frameIndex].end();
}

// Sets an image memory barrier to synchronize access to the image resource
// and changes its layout for optimal usage.
// This ensures writes complete before reads.
// Note this is also being used to prepare an image to be sampled
// in the fragment shader, effectively making the fragment stage wait for the 
// image to be ready. VULKAN IS EXPLICIT!!!
void Graphics::transitionImageLayout( 
                        const CommandBuffer &commandBuffer, const vk::Image &image,
                        ImageLayout oldL, ImageLayout newL, 
                        AccessFlags2 srcAccessMask, AccessFlags2 dstAccessMask,
                        PipelineStageFlags2 srcStageMask, PipelineStageFlags2 dstStageMask,
                        vk::ImageAspectFlagBits imageAspectFlags,
                        uint32_t mipLevels,
                        uint32_t srcQueue, uint32_t dstQueue) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask           = srcStageMask,
        .srcAccessMask          = srcAccessMask,
        .dstStageMask           = dstStageMask,
        .dstAccessMask          = dstAccessMask,
        .oldLayout              = oldL,
        .newLayout              = newL,
        .srcQueueFamilyIndex    = srcQueue,
        .dstQueueFamilyIndex    = dstQueue,
        .image                  = image,
        .subresourceRange       = {
            .aspectMask     = imageAspectFlags,
            .baseMipLevel   = 0,
            .levelCount     = mipLevels,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };

    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags         = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier
    };

    commandBuffer.pipelineBarrier2(dependencyInfo);
}

void Graphics::updateUniformBuffer(uint32_t frameIndex) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time       = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{
        .model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view  = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj  = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChain.getExtent().width) / static_cast<float>(swapChain.getExtent().height), 0.1f, 10.0f)
    };

    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[frameIndex], &ubo, sizeof(ubo));
}


void Graphics::drawFrame() {
    vk::Result fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
    
    if (fenceResult != vk::Result::eSuccess) 
        throw std::runtime_error("failed to wait for fence!");
    
    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, presentCompleteSemaphores[frameIndex], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        while(Application::getInstance().getWindowState() == WindowState::WINDOW_NULL) { glfwWaitEvents(); }

        std::cout << "Swap chain is out of date. Recreating..." << '\n';
        
        device.waitIdle(); // Wait until resource is no longer being used before recreating
        swapChain = SwapChain(swapChain, surface, physicalDevice, device);
        createDepthResources();
        
        return;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    device.resetFences(*inFlightFences[frameIndex]);

    recordCommandBuffer(imageIndex);

    graphicsQueue.waitIdle();

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    
    updateUniformBuffer(frameIndex);

    const vk::SubmitInfo submitInfo {
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &*presentCompleteSemaphores[frameIndex],
        .pWaitDstStageMask    = &waitDestinationStageMask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*commandBuffers[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*renderFinishedSemaphores[frameIndex]
    };

    graphicsQueue.submit(submitInfo, *inFlightFences[frameIndex]);

    const vk::PresentInfoKHR presentInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*renderFinishedSemaphores[frameIndex],
        .swapchainCount     = 1,
        .pSwapchains        = &*swapChain.getInstance(),
        .pImageIndices      = &imageIndex
    };

    result = graphicsQueue.presentKHR(presentInfo);

    switch (result) {
		case vk::Result::eSuccess: break;
		case vk::Result::eSuboptimalKHR:
        case vk::Result::eErrorOutOfDateKHR:
            while(Application::getInstance().getWindowState() == WindowState::WINDOW_NULL) { glfwWaitEvents(); }

			std::cout << "vk::Queue::presentKHR returned eSuboptimal or eErrorOutOfDate. Reacreating swap chain... !\n";

            device.waitIdle(); // Wait until resource is no longer being used before recreating
            swapChain = SwapChain(swapChain, surface, physicalDevice, device);
            createDepthResources();
			break;
		default:
            std::cout << "Queue returned an unexpected result !\n";
			break;        // an unexpected result is returned!
	}

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

const Device& Graphics::getDevice() const & { return device; }

uint32_t Graphics::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}
