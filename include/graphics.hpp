#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <application.hpp>
#include <version.hpp>
#include <config.hpp>
#include <swapChainManager.hpp>
#include <graphicsPipeline.hpp>

#include <iostream>
#include <vulkan/vulkan_raii.hpp>

using std::cout;
using std::cerr;
using std::endl;

using vk::raii::Context;
using vk::raii::Instance;
using vk::raii::DebugUtilsMessengerEXT;
using vk::raii::PhysicalDevice;
using vk::raii::Device;
using vk::raii::Queue;
using vk::raii::SurfaceKHR;
using vk::raii::CommandPool;
using vk::raii::CommandBuffer;

class Graphics {
    public:
        static Graphics& getInstance();

        Graphics(const Graphics&) = delete;
        Graphics(Graphics&&) noexcept = delete;

        ~Graphics();

        Graphics& operator=(const Graphics&) = delete;
        Graphics& operator=(Graphics&&) noexcept = delete;
    private:
        Graphics();

        void createInstance();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSurface();
        void createCommandPool();

        const bool isDeviceSuitable(const PhysicalDevice &physicalDevice) const;

        Context context;
        Instance instance                 = nullptr;
        PhysicalDevice physicalDevice     = nullptr;
        Device device                     = nullptr;
        Queue queue                       = nullptr; // Graphics and presentation queue (for now)
        SurfaceKHR surface                = nullptr;
        SwapChainManager swapChainManager = nullptr;
        GraphicsPipeline graphicsPipeline = nullptr;

        const std::vector<char const*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };

    #if DEBUG_MODE
        void setupDebugMessenger();

        DebugUtilsMessengerEXT debugMessenger = nullptr;

        static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                              vk::DebugUtilsMessageTypeFlagsEXT             type,
                                                              const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                              void*                                         pUserData)
        { std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << '\n'; return vk::False; }
    #endif
};

#endif