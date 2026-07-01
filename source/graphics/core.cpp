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
        VkBool32 profileSupported;

        vpGetInstanceProfileSupport(nullptr, &profile, &profileSupported);

        if (!profileSupported)
            throw std::runtime_error(std::string(VP_KHR_ROADMAP_2022_NAME) + " is not supported!");

        Runtime::Application& app = Runtime::Application::getInstance();

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
            auto layerProperties = context.enumerateInstanceLayerProperties();
            std::vector<vk::ExtensionProperties> extensionProperties = context.enumerateInstanceExtensionProperties();
            
            // ENABLE DEBUG FEATURES
        #if DEBUG_MODE
            requiredExtensions.push_back(vk::EXTDebugUtilsExtensionName);
                
            cout << "Available device extensions: " << '\n';

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
            .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data()
        };

        VpInstanceCreateInfo profileCreateInfo {
            .pCreateInfo             = createInfo,
            .enabledFullProfileCount = 1,
            .pEnabledFullProfiles    = &profile
        };

        VkInstance rawInstance = nullptr;

        if (vpCreateInstance(&profileCreateInfo, nullptr, &rawInstance) != VkResult::VK_SUCCESS)
            throw std::runtime_error("Failed to create instance with profile: " + std::string(VP_KHR_ROADMAP_2022_NAME));

        instance = vk::raii::Instance(context, rawInstance);

        } catch (const vk::SystemError& err) {
            cerr << "Vulkan Error: " << err.what() << '\n';
        } catch (const std::exception&  err) {
            cerr << "Error: " << err.what() << '\n';
        }
    }

    void Core::createSurface() {
        VkSurfaceKHR _surface;

        if (glfwCreateWindowSurface(*instance, Runtime::Application::getInstance().getWindow(), nullptr, &_surface) != 0) 
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
            if (isGraphicsAndComputeQueue(queueFamilyProperties[qfpIndex].queueFlags) &&
                physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) {
                graphicsQueueIndex = qfpIndex;

                foundGraphicsAndPresentation = true;
            } else if (isDedicatedTransferQueue(queueFamilyProperties[qfpIndex].queueFlags)) {
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

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

        queueCreateInfos.push_back({ .queueFamilyIndex = graphicsQueueIndex, .queueCount = 1, .pQueuePriorities = &graphicsQueuePriority });

        if (foundDedicatedTransfer) 
            queueCreateInfos.push_back({ .queueFamilyIndex = transferQueueIndex, .queueCount = 1, .pQueuePriorities = &transferQueuePriority });

        requiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        vk::DeviceCreateInfo deviceCreateInfo {
            .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos       = queueCreateInfos.data(),
            .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtensions.size()),
            .ppEnabledExtensionNames = requiredDeviceExtensions.data()
        };

        VpDeviceCreateInfo createInfo {
            .pCreateInfo             = deviceCreateInfo,
            .enabledFullProfileCount = 1,
            .pEnabledFullProfiles    = &profile
        };

        VkDevice rawDevice = VK_NULL_HANDLE;

        if (vpCreateDevice(*physicalDevice, &createInfo, nullptr, &rawDevice) != VkResult::VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device with profile: " + std::string(VP_KHR_ROADMAP_2022_NAME));
        }

        device            = vk::raii::Device(physicalDevice, rawDevice);
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
            vk::CommandPoolCreateInfo transferPoolInfo {
                .flags              = vk::CommandPoolCreateFlagBits::eTransient,
                .queueFamilyIndex   = transferQueueIndex
            };

            transferCommandPool = vk::raii::CommandPool(device, transferPoolInfo);
        }
    }

    const bool Core::isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice) {
        auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        uint32_t apiVersion = physicalDevice.getProperties().apiVersion;
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

        VkBool32 supported = false;
        vpGetPhysicalDeviceProfileSupport(*instance, *physicalDevice, &profile, &supported);

        if (!supported)
            throw std::runtime_error(std::string(VP_KHR_ROADMAP_2022_NAME) + " is not supported on this device!");

        return supported;
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
    const vk::raii::CommandPool&    Core::getComputeCommandPool()  const & { return graphicsCommandPool; }
    const vk::raii::CommandPool&    Core::getGraphicsCommandPool() const & { return graphicsCommandPool; }
    const vk::raii::Queue&          Core::getTransferQueue() const & { return transferQueueIndex != vk::QueueFamilyIgnored ? transferQueue : graphicsQueue; }
    const vk::raii::Queue&          Core::getComputeQueue()  const & { return graphicsQueue; } // Right now we are guaranteeing that the graphics queue family also supports compute
    const vk::raii::Queue&          Core::getGraphicsQueue() const & { return graphicsQueue; }
    
    const uint32_t Core::getTransferQueueIndex() const { return transferQueueIndex; }
    const uint32_t Core::getComputeQueueIndex()  const { return graphicsQueueIndex; }
    const uint32_t Core::getGraphicsQueueIndex() const { return graphicsQueueIndex; }
    const bool     Core::hasDedicatedTransferQueue() const { return transferQueueIndex != vk::QueueFamilyIgnored; }

    const bool Core::isDedicatedTransferQueue(const vk::QueueFlags &queueFlags) const {
        return (queueFlags & vk::QueueFlagBits::eTransfer)  &&
              !(queueFlags & vk::QueueFlagBits::eCompute)   &&
              !(queueFlags & vk::QueueFlagBits::eGraphics);
    }

    const bool Core::isGraphicsAndComputeQueue(const vk::QueueFlags &queueFlags) const {
        return (queueFlags & vk::QueueFlagBits::eCompute)   &&
               (queueFlags & vk::QueueFlagBits::eGraphics);
    }

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