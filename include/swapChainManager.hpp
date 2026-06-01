#ifndef SWAP_CHAIN_MANAGER_HPP

#include <application.hpp>

#include <iostream>
#include <cstdint>
#include <limits>
#include <algorithm>

#include <vulkan/vulkan_raii.hpp>

using std::vector;
using std::move;
using std::swap;

using vk::raii::PhysicalDevice;
using vk::raii::Device;
using vk::raii::SurfaceKHR;
using vk::raii::SwapchainKHR;
using vk::SurfaceFormatKHR;
using vk::SurfaceCapabilitiesKHR;
using vk::Extent2D;
using vk::PresentModeKHR;

class SwapChainManager {
    public:
        SwapChainManager() = delete;

        SwapChainManager(const SwapChainManager&) = delete;
        SwapChainManager(SwapChainManager&&) noexcept; 

        SwapChainManager(const SurfaceKHR& surface, const PhysicalDevice &physicalDevice, const Device &device);
        SwapChainManager(std::nullptr_t) noexcept;

        ~SwapChainManager();

        SwapChainManager& operator=(const SwapChainManager&) = delete;
        SwapChainManager& operator=(SwapChainManager&& other) noexcept;
        SwapChainManager& operator=(std::nullptr_t) noexcept;

        static SurfaceFormatKHR chooseSwapSurfaceFormat(const vector<SurfaceFormatKHR> &availableFormats);
        static PresentModeKHR   chooseSwapPresentMode(const vector<PresentModeKHR> &availablePresentModes);
        static uint32_t         chooseSwapMinImageCount(const SurfaceCapabilitiesKHR &capabilities);
        static Extent2D         chooseSwapExtent(const SurfaceCapabilitiesKHR &capabilities);

        const SwapchainKHR&     getSwapChain()              const;
        const SurfaceFormatKHR& getSwapChainSurfaceFormat() const;
        const Extent2D&         getSwapChainExtent()        const;
    
    private:
        SwapchainKHR                swapChain = nullptr;
        vector<vk::Image>           swapChainImages;
        vector<vk::raii::ImageView> swapChainImageViews;
        SurfaceFormatKHR            swapChainSurfaceFormat;
        Extent2D                    swapChainExtent;
};

#endif