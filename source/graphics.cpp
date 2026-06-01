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
    
    swapChainManager = SwapChainManager(surface, physicalDevice, device);
    graphicsPipeline = GraphicsPipeline(device, swapChainManager.getSwapChainExtent(), swapChainManager.getSwapChainSurfaceFormat());

    createCommandPool();
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
        
    auto features                 = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
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

    uint32_t queueIndex = ~0;

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

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        {},
        { .dynamicRendering = true },
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

}