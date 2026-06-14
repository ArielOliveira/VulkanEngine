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

#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

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
using vk::raii::Buffer;
using vk::raii::DeviceMemory;
using vk::raii::DescriptorPool;
using vk::raii::DescriptorSet;

using vk::ImageLayout;
using vk::AccessFlags2;
using vk::PipelineStageFlags2;

const std::string MODEL_PATH   = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

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
        void createDepthResources();
        void loadModel();
        void createVertexBuffer();
        void createIndexBuffer();
        void createTextureImage();
        void createTextureSampler();
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();
        void recordCommandBuffer(uint32_t imageIndex);
        void updateUniformBuffer(uint32_t imageIndex);

        void transitionImageLayout(const CommandBuffer &commandBuffer, const vk::Image &image,
                                   ImageLayout oldL, ImageLayout newL, 
                                   AccessFlags2 srcAccessMask, AccessFlags2 dstAccessMask,
                                   PipelineStageFlags2 srcStageMask, PipelineStageFlags2 dstStageMask,
                                   vk::ImageAspectFlagBits imageAspectFlags,
                                   uint32_t mipLevels,
                                   uint32_t srcQueue = vk::QueueFamilyIgnored, uint32_t dstQueue = vk::QueueFamilyIgnored);
        
        void generateMipmaps(const vk::Image &image, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels,
                             vk::Format imageFormat, vk::ImageTiling tiling, vk::ImageUsageFlags usageFlags, vk::MemoryPropertyFlagBits memoryType);

        void copyBuffer(Buffer &srcBuffer, Buffer &dstBuffer, vk::DeviceSize size); 
        void copyBufferToImage(CommandBuffer &commandBuffer, const Buffer &buffer, const vk::Image &image, uint32_t width, uint32_t height);
        
        CommandBuffer beginSingleTimeCommand(const CommandPool &commandPool);
        void endSingleTimeCommand(CommandBuffer &&commandBuffer, const Queue &queue);
            
        std::pair<Buffer, DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::SharingMode sharingMode = vk::SharingMode::eExclusive, uint32_t queueCount = 0, const uint32_t* queueIndices = nullptr);
        std::pair<vk::raii::Image, DeviceMemory> createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::SharingMode sharingMode = vk::SharingMode::eExclusive, uint32_t queueCount = 0, const uint32_t* queueIndices = nullptr);
        vk::raii::ImageView createImageView(const vk::Image &image, vk::Format format, vk::ImageAspectFlagBits aspectMaskFlags, uint32_t mipLevels);
        const bool isDeviceSuitable(const PhysicalDevice &physicalDevice) const;
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
        vk::Format findDepthFormat();
        vk::Format findSupportedFormat(const vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
        
        Context context;
        Instance instance                                   = nullptr;
        PhysicalDevice physicalDevice                       = nullptr;
        Device device                                       = nullptr;
        Queue graphicsQueue                                 = nullptr; // Graphics and presentation queue (for now)
        Queue transferQueue                                 = nullptr;
        SurfaceKHR surface                                  = nullptr;
        SwapChain swapChain                                 = nullptr;
        GraphicsPipeline graphicsPipeline                   = nullptr;
        CommandPool graphicsCommandPool                     = nullptr;
        CommandPool transferCommandPool                     = nullptr; // Using dedicated pools for short-lived command buffers allow for certain optimizations.
        Buffer vertexBuffer                                 = nullptr;
        Buffer indexBuffer                                  = nullptr;
        DeviceMemory vertexBufferMemory                     = nullptr;
        DeviceMemory indexBufferMemory                      = nullptr;

        vk::raii::Image        textureImage                 = nullptr;
        vk::raii::ImageView    textureImageView             = nullptr;
        vk::raii::Sampler      textureSampler               = nullptr;
        vk::raii::DeviceMemory textureImageMemory           = nullptr;
        uint32_t               textureMipCount;

        vk::raii::Image        depthImage                   = nullptr;
        vk::raii::DeviceMemory depthImageMemory             = nullptr;
        vk::raii::ImageView    depthImageView               = nullptr;

        vector<Vertex> vertices;
        vector<uint32_t> indices;

        vector<Buffer>       uniformBuffers;
        vector<DeviceMemory> uniformBuffersMemory;
        vector<void*>        uniformBuffersMapped;

        vector<CommandBuffer> commandBuffers;
        vector<Semaphore> presentCompleteSemaphores;
        vector<Semaphore> renderFinishedSemaphores;
        vector<Fence> inFlightFences;

        uint32_t graphicsQueueIndex               = ~0;
        uint32_t transferQueueIndex               = ~0;
        uint32_t frameIndex                       =  0;

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