#ifndef CORE_HPP
#define CORE_HPP

#include <vulkan/vulkan_raii.hpp>

#include <runtime/application.hpp>

#include <iostream>

using std::cout;

namespace Graphics {
    class Core {
        public:
            static Core& getInstance();

            Core(const Core&) = delete;
            Core(Core&&) noexcept = delete;

            Core& operator=(const Core&) = delete;
            Core& operator=(Core&&) noexcept = delete;

            ~Core();

            std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::SharingMode sharingMode = vk::SharingMode::eExclusive, uint32_t queueCount = 0, const uint32_t* queueIndices = nullptr);

            const vk::SampleCountFlagBits getMaxUsableSampleCount() const;

            const vk::Format findDepthFormat() const;
            const vk::Format findSupportedFormat(const vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const;
            const uint32_t   findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

            const vk::raii::SurfaceKHR&     getSurface()             const &;
            const vk::raii::Device&         getDevice()              const &;
            const vk::raii::PhysicalDevice& getPhysicalDevice()      const &;
            const vk::raii::CommandPool&    getTransferCommandPool() const &;
            const vk::raii::CommandPool&    getComputeCommandPool()  const &;
            const vk::raii::CommandPool&    getGraphicsCommandPool() const &;
            const vk::raii::Queue&          getTransferQueue()       const &;
            const vk::raii::Queue&          getComputeQueue()        const &;
            const vk::raii::Queue&          getGraphicsQueue()       const &;

            const uint32_t getTransferQueueIndex() const;
            const uint32_t getComputeQueueIndex()  const;
            const uint32_t getGraphicsQueueIndex() const;
            const bool     hasDedicatedTransferQueue() const;
        private:
            vk::raii::Context context;
            vk::raii::Instance instance                                   = nullptr;
            vk::raii::PhysicalDevice physicalDevice                       = nullptr;
            vk::raii::Device device                                       = nullptr;
            vk::raii::Queue graphicsQueue                                 = nullptr; // Graphics and presentation queue (for now)
            vk::raii::Queue transferQueue                                 = nullptr;
            vk::raii::SurfaceKHR surface                                  = nullptr;
             
            vk::raii::CommandPool graphicsCommandPool                     = nullptr;
            vk::raii::CommandPool transferCommandPool                     = nullptr; // Using dedicated pools for short-lived command buffers allow for certain optimizations.

            uint32_t graphicsQueueIndex               = vk::QueueFamilyIgnored;
            uint32_t transferQueueIndex               = vk::QueueFamilyIgnored;

            const std::vector<char const*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

            Core();

            void createInstance();
            void pickPhysicalDevice();
            void createLogicalDevice();
            void createSurface();
            void createCommandPool();

            const bool isDeviceSuitable(const vk::raii::PhysicalDevice &physicalDevice) const;

        #if DEBUG_MODE
            void setupDebugMessenger();

            vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;

            static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                                vk::DebugUtilsMessageTypeFlagsEXT             type,
                                                                const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                                void*                                         pUserData)
            { std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << '\n'; return vk::False; }
        #endif
    };
}

#endif

