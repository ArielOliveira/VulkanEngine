#ifndef GRAPHICS_TEXTURE_HPP
#define GRAPHICS_TEXTURE_HPP

#include <graphics/core.hpp>
#include <graphics/commandBuffer.hpp>
#include <graphics/swapChain.hpp>

const std::string TEXTURE_PATH = "textures/";
namespace Graphics {
    class Texture {
        public:
            Texture() = delete;
            Texture(std::nullptr_t) noexcept;
            Texture(const Texture&) = delete;
            Texture(Texture&& rhs) noexcept = default;
            Texture(const std::string& name, vk::Format format, vk::ImageTiling tiling, vk::ImageAspectFlags aspectFlags, vk::ImageUsageFlags usageFlags, vk::SampleCountFlagBits msaaSamples, uint32_t width, uint32_t height, uint32_t channels, bool generateMips);
            Texture(const std::string& name, vk::Format format, vk::ImageTiling tiling, vk::ImageAspectFlags aspectFlags, vk::ImageUsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t channels, bool generateMips);
            Texture(const std::string& name, vk::Format format, vk::ImageTiling tiling, vk::ImageAspectFlags aspectFlags, vk::ImageUsageFlags usageFlags);

            static Texture createColorResolve(const SwapChain &swapChain);
            static Texture createDepthBuffer(const SwapChain &swapChain);

            ~Texture();

            void cleanUp();
            
            Texture& operator=(std::nullptr_t) noexcept;
            Texture& operator=(const Texture& rhs) = delete;
            Texture& operator=(Texture&& rhs);

            static void updateImageLayout(const CommandBuffer &commandBuffer, const vk::ImageMemoryBarrier2 &barrier, const uint32_t commandIndex = 0);
            //static void copyBufferToImage(CommandBuffer &commandBuffer, const Buffer &buffer, const vk::Image &image, vk::ImageAspectFlags aspectFlags, vk::ImageLayout layout, uint32_t width, uint32_t height);

            static vk::raii::ImageView createImageView(const vk::Image &image, vk::Format format, vk::ImageAspectFlags aspectMaskFlags, uint32_t mipLevels);
            static std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(vk::ImageCreateInfo &imageInfo, vk::MemoryPropertyFlags memoryProperties);

            const vk::raii::Image&     getImage() const;
            const vk::raii::ImageView& getImageView() const;
            const vk::raii::Sampler&   getSampler() const;

            void updateImageLayout(const CommandBuffer &commandBuffer, vk::ImageLayout targetLayout, vk::AccessFlags2 targetAccess, vk::PipelineStageFlags2 targetStage, uint32_t commandIndex = 0, bool ignoreSourceLayout = false);
            void updateImageLayout(const CommandBuffer &commandBuffer, vk::ImageLayout targetLayout, vk::AccessFlags2 targetAccess, vk::PipelineStageFlags2 targetStage, uint32_t targetQueue = vk::QueueFamilyIgnored, uint32_t commandIndex = 0, bool ignoreSourceLayout = false);
        private:
            vk::raii::Image image         = nullptr;
            vk::raii::ImageView imageView = nullptr;
            vk::raii::DeviceMemory memory = nullptr;
            vk::raii::Sampler sampler     = nullptr;
            vk::DeviceSize offset;
            
            vk::Format format;
            vk::ImageTiling tiling;
            vk::ImageAspectFlags aspectFlags;
            vk::ImageUsageFlags usageFlags;

            vk::ImageLayout currentLayout           = vk::ImageLayout::eUndefined;
            vk::AccessFlagBits2 currentAccess       = vk::AccessFlagBits2::eNone;
            vk::PipelineStageFlagBits2 currentStage = vk::PipelineStageFlagBits2::eTopOfPipe;
            vk::SampleCountFlagBits msaaSamples     = vk::SampleCountFlagBits::e1;

            std::string name;

            uint32_t width             = 0;
            uint32_t height            = 0;
            uint32_t channels          = 0;
            uint32_t mipCount          = 0;
            uint32_t currentQueueIndex = vk::QueueFamilyIgnored;

            void copyBufferToImage(const CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer);
            void createImage();
            void createImageView();
            void createSampler();
            void generateMipmaps();
    };
}
#endif