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
using vk::raii::Semaphore;
using vk::raii::Fence;
using vk::SurfaceFormatKHR;
using vk::SurfaceCapabilitiesKHR;
using vk::Extent2D;
using vk::PresentModeKHR;

class SwapChain {
    public:
        SwapChain() = delete;

        SwapChain(const SwapChain&) = delete;
        SwapChain(SwapChain&&) noexcept = default; 

        SwapChain(const SurfaceKHR& surface, const PhysicalDevice &physicalDevice, const Device &device);
        SwapChain(SwapChain& old, const SurfaceKHR& surface, const PhysicalDevice &physicalDevice, const Device &device);
        SwapChain(std::nullptr_t) noexcept;

        ~SwapChain();

        SwapChain& operator=(const SwapChain&) = delete;
        SwapChain& operator=(SwapChain&& other) noexcept = default;
        SwapChain& operator=(std::nullptr_t) noexcept;

        static SurfaceFormatKHR chooseSwapSurfaceFormat(const vector<SurfaceFormatKHR> &availableFormats);
        static PresentModeKHR   chooseSwapPresentMode(const vector<PresentModeKHR> &availablePresentModes);
        static uint32_t         chooseSwapMinImageCount(const SurfaceCapabilitiesKHR &capabilities);
        static Extent2D         chooseSwapExtent(const SurfaceCapabilitiesKHR &capabilities);

        const SwapchainKHR&     getInstance()                const&;
        const SurfaceFormatKHR& getSurfaceFormat()           const;
        const Extent2D&         getExtent()                  const;
        const vk::Image&        getImage(uint32_t index)     const;
        const vk::ImageView&    getImageView(uint32_t index) const;
        const size_t getImageCount() const;

        const vk::ResultValue<uint32_t> acquireNextImage(uint64_t timeout, const Semaphore &semaphore, const Fence &fence) const;
        
    
    private:
        SwapchainKHR                swapChain = nullptr;
        vector<vk::Image>           swapChainImages;
        vector<vk::raii::ImageView> swapChainImageViews;
        SurfaceFormatKHR            swapChainSurfaceFormat;
        Extent2D                    swapChainExtent;

        void cleanUp();
        void buildSwapChain(const SurfaceKHR &surface, const PhysicalDevice &physicalDevice, const Device &device);
};

#endif