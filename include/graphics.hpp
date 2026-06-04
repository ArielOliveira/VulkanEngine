#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS

#include <application.hpp>
#include <version.hpp>
#include <config.hpp>
#include <swapChain.hpp>
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
using vk::raii::Semaphore;
using vk::raii::Fence;

using vk::ImageLayout;
using vk::AccessFlags2;
using vk::PipelineStageFlags2;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class Graphics {
    public:
        static Graphics& getInstance();

        Graphics(const Graphics&) = delete;
        Graphics(Graphics&&) noexcept = delete;

        ~Graphics();

        Graphics& operator=(const Graphics&) = delete;
        Graphics& operator=(Graphics&&) noexcept = delete;

        void drawFrame();

        const Device& getDevice() const &;
    private:
        Graphics();

        void createInstance();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSurface();
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();
        void recordCommandBuffer(uint32_t imageIndex);

        void transitionImageLayout(uint32_t imageIndex, 
                                  ImageLayout oldL, ImageLayout newL, 
                                  AccessFlags2 srcAccessMask, AccessFlags2 dstAccessMask,
                                  PipelineStageFlags2 srcStageMask, PipelineStageFlags2 dstStageMask);

        const bool isDeviceSuitable(const PhysicalDevice &physicalDevice) const;

        Context context;
        Instance instance                           = nullptr;
        PhysicalDevice physicalDevice               = nullptr;
        Device device                               = nullptr;
        Queue queue                                 = nullptr; // Graphics and presentation queue (for now)
        SurfaceKHR surface                          = nullptr;
        SwapChain swapChain                         = nullptr;
        GraphicsPipeline graphicsPipeline           = nullptr;
        CommandPool commandPool                     = nullptr;

        vector<CommandBuffer> commandBuffers;
        vector<Semaphore> presentCompleteSemaphores;
        vector<Semaphore> renderFinishedSemaphores;
        vector<Fence> inFlightFences;

        uint32_t queueIndex               = ~0;
        uint32_t frameIndex               =  0;

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