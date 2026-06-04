#include <swapChain.hpp>

SwapChain::SwapChain(const SurfaceKHR &surface, const PhysicalDevice &physicalDevice, const Device &device) {
    buildSwapChain(surface, physicalDevice, device);
}

SwapChain::SwapChain(SwapChain& old, const SurfaceKHR& surface, const PhysicalDevice &physicalDevice, const Device &device) {
    old.cleanUp();
    buildSwapChain(surface, physicalDevice, device);
}

void SwapChain::buildSwapChain(const SurfaceKHR &surface, const PhysicalDevice &physicalDevice, const Device &device) {
    SurfaceCapabilitiesKHR surfaceCapabilities   = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    vector<SurfaceFormatKHR> availableFormats    = physicalDevice.getSurfaceFormatsKHR(*surface);
    vector<PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
    
    swapChainExtent                            = chooseSwapExtent(surfaceCapabilities);
    uint32_t minImageCount                     = chooseSwapMinImageCount(surfaceCapabilities);
    swapChainSurfaceFormat                     = chooseSwapSurfaceFormat(availableFormats);
    PresentModeKHR presentMode                 = chooseSwapPresentMode(availablePresentModes);

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

    swapChain       = SwapchainKHR(device, swapChainCreateInfo);
    swapChainImages = swapChain.getImages();

    // Create Image Views
    assert(swapChainImageViews.empty());

    vk::ImageViewCreateInfo imageViewCreateInfo {
        .viewType         = vk::ImageViewType::e2D,
        .format           = swapChainSurfaceFormat.format,
        .components       = { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity },
        .subresourceRange = { .aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1 }
    };

    for (auto &image : swapChainImages) {
        imageViewCreateInfo.image = image;
        swapChainImageViews.emplace_back(device, imageViewCreateInfo);
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
    swapChain.clear();
    swapChainImages.clear();
    swapChainImageViews.clear();
    swapChain = nullptr;
}

SurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const vector<SurfaceFormatKHR> &availableFormats) {
    assert(!availableFormats.empty());

    const auto formatIt = std::ranges::find_if(
        availableFormats,
        [](const auto &format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; }
    );

    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

PresentModeKHR SwapChain::chooseSwapPresentMode(const vector<PresentModeKHR> &availablePresentModes) {
    assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
    
    return std::ranges::any_of(availablePresentModes,
                               [](const PresentModeKHR value) { return PresentModeKHR::eMailbox == value; }) ?
                               PresentModeKHR::eMailbox :
                               PresentModeKHR::eFifo;
}

uint32_t SwapChain::chooseSwapMinImageCount(const SurfaceCapabilitiesKHR &capabilities) {
    auto minImageCount = std::max(3u, capabilities.minImageCount);
    
    if ((0 < capabilities.maxImageCount) && (capabilities.maxImageCount < minImageCount)) 
        minImageCount = capabilities.maxImageCount;

    return minImageCount;
}

Extent2D SwapChain::chooseSwapExtent(const SurfaceCapabilitiesKHR &capabilities) {
    // If currentExtent is not set to the max value of uint32_t, it means
    // the current window resolution is already fixed?
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    
    int width, height;

    Application::getInstance().getFramebufferSize(&width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

const SwapchainKHR& SwapChain::getInstance() const & { return swapChain; }
const SurfaceFormatKHR& SwapChain::getSurfaceFormat() const { return swapChainSurfaceFormat; }
const Extent2D& SwapChain::getExtent() const { return swapChainExtent; }
const size_t SwapChain::getImageCount() const { return swapChainImages.size(); }

const vk::Image& SwapChain::getImage(uint32_t index) const {
    if (index < 0 || index > swapChainImages.size()-1)
        throw std::runtime_error("Invalid Swapchain image index!");

    return swapChainImages[index];
}

const vk::ImageView& SwapChain::getImageView(uint32_t index) const {
    if (index < 0 || index > swapChainImageViews.size()-1)
        throw std::runtime_error("Invalid Swapchain image index!");

    return *swapChainImageViews[index];
}

const vk::ResultValue<uint32_t> SwapChain::acquireNextImage(uint64_t timeout, const Semaphore &semaphore, const Fence &fence) const { return swapChain.acquireNextImage(timeout, semaphore, fence); }