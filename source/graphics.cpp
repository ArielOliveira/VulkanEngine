#include <graphics.hpp>

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
    graphicsPipeline = GraphicsPipeline(device, swapChain.getExtent(), swapChain.getSurfaceFormat());

    createCommandPool();
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

    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) {
            queueIndex = qfpIndex;
            break;
        }
    }

    if (queueIndex == ~0)
        throw std::runtime_error("Could not find a queue for graphics and presentation when creating logical device -> terminating");
    
    float queuePriority = 0.5f;  

    vk::PhysicalDeviceFeatures deviceFeatures;

    vk::StructureChain<
            vk::PhysicalDeviceFeatures2, 
            vk::PhysicalDeviceVulkan11Features, 
            vk::PhysicalDeviceVulkan13Features, 
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        {},
        { .shaderDrawParameters = true }, 
        { .synchronization2 = true,
          .dynamicRendering = true},
        { .extendedDynamicState = true }
    };

    vector<const char*> requiredDeviceExtensions = {
        vk::KHRSwapchainExtensionName
    };

    vk::DeviceQueueCreateInfo deviceQueueCreateInfo { 
        .queueFamilyIndex = queueIndex,
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority
    };

    vk::DeviceCreateInfo deviceCreateInfo {
        .pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &deviceQueueCreateInfo,
        .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = requiredDeviceExtensions.data()
    };

    device = Device(physicalDevice, deviceCreateInfo);
    queue  = Queue(device, queueIndex, 0);
}

void Graphics::createSurface() {
    VkSurfaceKHR _surface;

    if (glfwCreateWindowSurface(*instance, Application::getInstance().getWindow(), nullptr, &_surface) != 0) 
        throw std::runtime_error("failed to create window surface");

    surface = SurfaceKHR(instance, _surface);
}

void Graphics::createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo {
        .flags              = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex   = queueIndex
    };

    commandPool = CommandPool(device, poolInfo);
}

void Graphics::createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocInfo {
        .commandPool        = commandPool,
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

void Graphics::recordCommandBuffer(uint32_t imageIndex) {
    commandBuffers[frameIndex].begin({});

    transitionImageLayout(
        imageIndex,
        ImageLayout::eUndefined,
        ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);

    vk::RenderingAttachmentInfo attachmentInfo {
        .imageView      = swapChain.getImageView(imageIndex),
        .imageLayout    = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp         = vk::AttachmentLoadOp::eClear,
        .storeOp        = vk::AttachmentStoreOp::eStore,
        .clearValue     = clearColor
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea           = {.offset = {0, 0}, .extent = swapChain.getExtent()},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &attachmentInfo
    };

    commandBuffers[frameIndex].beginRendering(renderingInfo);
    commandBuffers[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline.getInstance());
    commandBuffers[frameIndex].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.getExtent().width), static_cast<float>(swapChain.getExtent().height), 0.0f, 1.0f));
    commandBuffers[frameIndex].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.getExtent()));
    commandBuffers[frameIndex].draw(3, 1, 0, 0);
    commandBuffers[frameIndex].endRendering();

    transitionImageLayout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    commandBuffers[frameIndex].end();
}

void Graphics::transitionImageLayout(
                        uint32_t imageIndex, 
                        ImageLayout oldL, ImageLayout newL, 
                        AccessFlags2 srcAccessMask, AccessFlags2 dstAccessMask,
                        PipelineStageFlags2 srcStageMask, PipelineStageFlags2 dstStageMask) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask           = srcStageMask,
        .srcAccessMask          = srcAccessMask,
        .dstStageMask           = dstStageMask,
        .dstAccessMask          = dstAccessMask,
        .oldLayout              = oldL,
        .newLayout              = newL,
        .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
        .image                  = swapChain.getImage(imageIndex),
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

    commandBuffers[frameIndex].pipelineBarrier2(dependencyInfo);
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
        
        return;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    device.resetFences(*inFlightFences[frameIndex]);

    recordCommandBuffer(imageIndex);

    queue.waitIdle();

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    
    const vk::SubmitInfo submitInfo {
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &*presentCompleteSemaphores[frameIndex],
        .pWaitDstStageMask    = &waitDestinationStageMask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*commandBuffers[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*renderFinishedSemaphores[frameIndex]
    };

    queue.submit(submitInfo, *inFlightFences[frameIndex]);

    const vk::PresentInfoKHR presentInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*renderFinishedSemaphores[frameIndex],
        .swapchainCount     = 1,
        .pSwapchains        = &*swapChain.getInstance(),
        .pImageIndices      = &imageIndex
    };

    result = queue.presentKHR(presentInfo);

    switch (result) {
		case vk::Result::eSuccess: break;
		case vk::Result::eSuboptimalKHR:
        case vk::Result::eErrorOutOfDateKHR:
            while(Application::getInstance().getWindowState() == WindowState::WINDOW_NULL) { glfwWaitEvents(); }

			std::cout << "vk::Queue::presentKHR returned eSuboptimal or eErrorOutOfDate. Reacreating swap chain... !\n";

            swapChain = SwapChain(swapChain, surface, physicalDevice, device);
			break;
		default:
            std::cout << "Queue returned an unexpected result !\n";
			break;        // an unexpected result is returned!
	}

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

const Device& Graphics::getDevice() const & { return device; }
