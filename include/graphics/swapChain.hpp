#ifndef GRAPHICS_SWAPCHAIN_HPP
#define GRAPHICS_SWAPCHAIN_HPP

#include <runtime/application.hpp>

#include <iostream>
#include <cstdint>
#include <limits>
#include <algorithm>

#include <vulkan/vulkan_raii.hpp>

#include <engine/resources.hpp>

using std::vector;
using std::move;
using std::swap;

using Engine::ResourceHandle;
using Engine::Resources::Image;

namespace Graphics {
    class SwapChain {
        public:
            SwapChain() = delete;

            SwapChain(const SwapChain&) = delete;
            SwapChain(SwapChain&&) noexcept = default; 

            SwapChain(const vk::raii::SurfaceKHR& surface, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device);
            SwapChain(SwapChain& old, const vk::raii::SurfaceKHR& surface, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device);
            SwapChain(std::nullptr_t) noexcept;

            ~SwapChain();

            SwapChain& operator=(const SwapChain&) = delete;
            SwapChain& operator=(SwapChain&& other) noexcept = default;
            SwapChain& operator=(std::nullptr_t) noexcept;

            const ResourceHandle<Image>&  getImage(uint32_t index)     const;
            const vk::raii::SwapchainKHR& getInstance()                const&;
            const vk::SurfaceFormatKHR&   getSurfaceFormat()           const;
            const vk::Extent2D&           getExtent()                  const;
            const size_t                  getImageCount()              const;

            const vk::ResultValue<uint32_t> acquireNextImage(uint64_t timeout, const vk::raii::Semaphore &semaphore, const vk::raii::Fence &fence) const;  
        
        private:
            vk::raii::SwapchainKHR            swapChain = nullptr;
            vector<ResourceHandle<Image>>     images;

            vk::SurfaceFormatKHR              swapChainSurfaceFormat;
            vk::Extent2D                      swapChainExtent;

            void cleanUp();
            void buildSwapChain(const vk::raii::SurfaceKHR &surface, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device);

            static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const vector<vk::SurfaceFormatKHR> &availableFormats);
            static vk::PresentModeKHR   chooseSwapPresentMode(const vector<vk::PresentModeKHR> &availablePresentModes);
            static uint32_t             chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR &capabilities);
            static vk::Extent2D         chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);
    };
}

#endif