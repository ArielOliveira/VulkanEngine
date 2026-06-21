#include <graphics/core.hpp>

namespace Graphics {
    Core& Core::getInstance() {
        static Core instance;

        return instance;
    }

    Core::Core() {
        cout << "Initializing graphics..." << '\n';

        createInstance();
    #if DEBUG_MODE 
        setupDebugMessenger(); 
    #endif
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSurface();
        createCommandPool();
    }

    Core::~Core() {}

    void Core::createInstance() {
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

        instance = vk::raii::Instance(context, createInfo);

        } catch (const vk::SystemError& err) {
            cerr << "Vulkan Error: " << err.what() << '\n';
        } catch (const std::exception&  err) {
            cerr << "Error: " << err.what() << '\n';
        }
    }

    void Core::createSurface() {
        VkSurfaceKHR _surface;

        if (glfwCreateWindowSurface(*instance, Application::getInstance().getWindow(), nullptr, &_surface) != 0) 
            throw std::runtime_error("failed to create window surface");

        surface = vk::raii::SurfaceKHR(instance, _surface);
    }

    void Core::pickPhysicalDevice() {
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

    void Core::createLogicalDevice() {
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

        device            = vk::raii::Device(physicalDevice, deviceCreateInfo);
        graphicsQueue     = vk::raii::Queue(device, graphicsQueueIndex, 0);
        if (foundDedicatedTransfer)
            transferQueue = vk::raii::Queue(device, transferQueueIndex, 0);
    }

    void Core::createCommandPool() {
        vk::CommandPoolCreateInfo graphicsPoolInfo {
            .flags              = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex   = graphicsQueueIndex
        };

        graphicsCommandPool = vk::raii::CommandPool(device, graphicsPoolInfo);
        
        if (hasDedicatedTransferQueue()) {
            // Review flags usage. Tutorial suggests using only eTransient, while AI chatbot (shame on me)
            // mentions short-lived command buffers (i.e transfers) will get reset often (?) so eResetCommandBuffer 
            // is also appropriate.
            vk::CommandPoolCreateInfo transferPoolInfo {
                .flags              = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                .queueFamilyIndex   = transferQueueIndex
            };

            transferCommandPool = vk::raii::CommandPool(device, transferPoolInfo);
        }
    }

    const bool Core::isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice) const {
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
            
        auto features                 = 
            physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2, 
                                                 vk::PhysicalDeviceVulkan11Features, 
                                                 vk::PhysicalDeviceVulkan13Features, 
                                                 vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
            
        bool supportsRequiredFeatures = 
            features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
            features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
            features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
            features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        return supportsVulkan1_3 && supportsGraphicsAndPresentation && supportsAllRequiredExtensions && supportsRequiredFeatures;
    }

    const vk::Format Core::findDepthFormat() const {
        return findSupportedFormat(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );
    }

    const vk::Format Core::findSupportedFormat(const vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const {
        for (const auto format : candidates) {
            vk::FormatProperties props = physicalDevice.getFormatProperties(format);

            if (((tiling == vk::ImageTiling::eLinear) && ((props.linearTilingFeatures & features) == features)) ||
                ((tiling == vk::ImageTiling::eOptimal) && ((props.optimalTilingFeatures & features) == features))) {
                    return format;
            }
        }
        
        throw std::runtime_error("failed to find supported format!");
    }
    
    const uint32_t Core::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const { vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!"); 
    }

    const vk::SampleCountFlagBits Core::getMaxUsableSampleCount() const {
        vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();

        vk::SampleCountFlags counts = deviceProperties.limits.framebufferColorSampleCounts & deviceProperties.limits.framebufferDepthSampleCounts;
        
        if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
        if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
        if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
        if (counts & vk::SampleCountFlagBits::e8)  { return vk::SampleCountFlagBits::e8; }
        if (counts & vk::SampleCountFlagBits::e4)  { return vk::SampleCountFlagBits::e4; }
        if (counts & vk::SampleCountFlagBits::e2)  { return vk::SampleCountFlagBits::e2; }

        return vk::SampleCountFlagBits::e1;
    }
    
    const vk::raii::SurfaceKHR&     Core::getSurface()             const & { return surface; }
    const vk::raii::Device&         Core::getDevice() const & { return device; }
    const vk::raii::PhysicalDevice& Core::getPhysicalDevice() const & { return physicalDevice; }
    const vk::raii::CommandPool&    Core::getTransferCommandPool() const & { return transferQueueIndex != vk::QueueFamilyIgnored ? transferCommandPool : graphicsCommandPool; }
    const vk::raii::CommandPool&    Core::getGraphicsCommandPool() const & { return graphicsCommandPool; }
    const vk::raii::Queue&          Core::getTransferQueue() const & { return transferQueueIndex != vk::QueueFamilyIgnored ? transferQueue : graphicsQueue; }
    const vk::raii::Queue&          Core::getGraphicsQueue() const & { return graphicsQueue; }
    
    const uint32_t Core::getGraphicsQueueIndex() const { return graphicsQueueIndex;}
    const uint32_t Core::getTransferQueueIndex() const { return transferQueueIndex; }
    const bool     Core::hasDedicatedTransferQueue() const { return transferQueueIndex != vk::QueueFamilyIgnored; }

#if DEBUG_MODE
    void Core::setupDebugMessenger() {
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

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> Core::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::SharingMode sharingMode, uint32_t queueCount, const uint32_t* queueIndices) {
        vk::BufferCreateInfo bufferInfo {
            .size                   = size,
            .usage                  = usage,
            .sharingMode            = sharingMode,
            .queueFamilyIndexCount  = queueCount,
            .pQueueFamilyIndices    = queueIndices
        };

        vk::raii::Buffer buffer = vk::raii::Buffer(device, bufferInfo);
        vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
        vk::MemoryAllocateInfo memAllocateInfo = { 
            .allocationSize = memRequirements.size, 
            .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties) 
        };

        vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, memAllocateInfo);
        buffer.bindMemory(*bufferMemory, 0);

        return { std::move(buffer), std::move(bufferMemory) };
    }
}