#include <graphics/swapChain.hpp>

#include <engine/resourceEngine.hpp>

using Engine::GPUResourceManager;

namespace Graphics {
    SwapChain::SwapChain(const vk::raii::SurfaceKHR& surface, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device) {
        buildSwapChain(surface, physicalDevice, device);
    }

    SwapChain::SwapChain(SwapChain& old, const vk::raii::SurfaceKHR& surface, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device) {
        old.cleanUp();
        buildSwapChain(surface, physicalDevice, device);
    }

    void SwapChain::buildSwapChain(const vk::raii::SurfaceKHR &surface, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device) {
        vk::SurfaceCapabilitiesKHR surfaceCapabilities   = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
        vector<vk::SurfaceFormatKHR> availableFormats    = physicalDevice.getSurfaceFormatsKHR(*surface);
        vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
        
        swapChainExtent                            = chooseSwapExtent(surfaceCapabilities);
        uint32_t minImageCount                     = chooseSwapMinImageCount(surfaceCapabilities);
        swapChainSurfaceFormat                     = chooseSwapSurfaceFormat(availableFormats);
        vk::PresentModeKHR presentMode             = chooseSwapPresentMode(availablePresentModes);

        vk::SwapchainCreateInfoKHR swapChainCreateInfo {
            .surface            = *surface,
            .minImageCount      = minImageCount,
            .imageFormat        = swapChainSurfaceFormat.format,
            .imageColorSpace    = swapChainSurfaceFormat.colorSpace,
            .imageExtent        = swapChainExtent,
            .imageArrayLayers   = 1,
            .imageUsage         = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode   = vk::SharingMode::eExclusive,
            .preTransform       = surfaceCapabilities.currentTransform,
            .compositeAlpha     = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode        = presentMode,
            .clipped            = true,
            .oldSwapchain       = nullptr
        };

        swapChain                              = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
        std::vector<vk::Image> swapChainImages = swapChain.getImages();

        // Register images to resource manager
        assert(images.empty());

        images.reserve(swapChainImages.size());
        uint32_t imageId = 0;
        for (auto &image : swapChainImages) {
            images.emplace_back(std::move(GPUResourceManager::getInstance().registerExternal(
                static_cast<VkImage>(image),
                VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
                static_cast<VkFormat>(swapChainSurfaceFormat.format),
                swapChainExtent.width, swapChainExtent.height, 1U
            )));
        }
    }

    SwapChain::SwapChain(std::nullptr_t) noexcept {}

    SwapChain::~SwapChain() {
        cleanUp();
    }

    SwapChain& SwapChain::operator=(std::nullptr_t) noexcept {
        cleanUp();

        return *this;
    }

    void SwapChain::cleanUp() {
        images.clear();
        swapChain.clear();
        swapChain = nullptr;
    }

    vk::SurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const vector<vk::SurfaceFormatKHR> &availableFormats) {
        assert(!availableFormats.empty());

        const auto formatIt = std::ranges::find_if(
            availableFormats,
            [](const auto &format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; }
        );

        return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
    }

    vk::PresentModeKHR SwapChain::chooseSwapPresentMode(const vector<vk::PresentModeKHR> &availablePresentModes) {
        assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
        
        return std::ranges::any_of(availablePresentModes,
                                [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
                                vk::PresentModeKHR::eMailbox :
                                vk::PresentModeKHR::eFifo;
    }

    uint32_t SwapChain::chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR &capabilities) {
        auto minImageCount = std::max(3u, capabilities.minImageCount);
        
        if ((0 < capabilities.maxImageCount) && (capabilities.maxImageCount < minImageCount)) 
            minImageCount = capabilities.maxImageCount;

        return minImageCount;
    }

    vk::Extent2D SwapChain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) {
        // If currentExtent is not set to the max value of uint32_t, it means
        // the current window resolution is already fixed?
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;
        
        int width, height;

        Runtime::Application::getInstance().getFramebufferSize(&width, &height);

        return {
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }

    const vk::raii::SwapchainKHR& SwapChain::getInstance() const & { return swapChain; }
    const vk::SurfaceFormatKHR& SwapChain::getSurfaceFormat() const { return swapChainSurfaceFormat; }
    const vk::Extent2D& SwapChain::getExtent() const { return swapChainExtent; }
    const size_t SwapChain::getImageCount() const { return images.size(); }

    const ResourceHandle<Image>& SwapChain::getImage(uint32_t index) const {
        if (index < 0 || index > images.size()-1)
            throw std::runtime_error("Invalid Swapchain image index!");

        return images[index];
    }

    const vk::ResultValue<uint32_t> SwapChain::acquireNextImage(uint64_t timeout, const vk::raii::Semaphore &semaphore, const vk::raii::Fence &fence) const { return swapChain.acquireNextImage(timeout, semaphore, fence); }
}