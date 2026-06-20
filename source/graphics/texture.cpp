#include <graphics/texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Graphics {
    Texture::Texture(const std::string& name, vk::Format format, vk::ImageTiling tiling, vk::ImageAspectFlags aspectFlags, vk::ImageUsageFlags usageFlags) :
        name(std::string(name)),
        format(format),
        tiling(tiling),
        aspectFlags(aspectFlags),
        usageFlags(usageFlags) {
            int _width, _height, _channels;
            stbi_uc *pixels             = stbi_load((TEXTURE_PATH + name).c_str(), &_width, &_height, &_channels, STBI_rgb_alpha);
            
            width    = _width;
            height   = _height;
            channels = _channels;

            vk::DeviceSize imageSize = width * height * 4;

            if (!pixels)
                throw std::runtime_error("Failed to load texture: " + name);

            auto [stagingBuffer, stagingBufferMemory] = Core::getInstance().createBuffer(
                imageSize,
                vk::BufferUsageFlagBits::eTransferSrc,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );

            void *data = stagingBufferMemory.mapMemory(0, imageSize);
            memcpy(data, pixels, imageSize);
            stagingBufferMemory.unmapMemory();

            stbi_image_free(pixels);
            mipCount = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

            createImage();

            Core& core       = Core::getInstance();
            CommandBuffer cb = CommandBuffer::singleTimeTransferCommand();
            
            updateImageLayout(cb, vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits2::eTransferWrite, vk::PipelineStageFlagBits2::eTransfer, core.getTransferQueueIndex());
            copyBufferToImage(cb, stagingBuffer);
            
            cb.dispatch();

            if (core.hasDedicatedTransferQueue()) {
                CommandBuffer cb0 = CommandBuffer::singleTimeTransferCommand();
                updateImageLayout(cb0, vk::ImageLayout::eTransferSrcOptimal, vk::AccessFlagBits2::eTransferRead, vk::PipelineStageFlagBits2::eTransfer, core.getGraphicsQueueIndex());
                cb0.dispatch();
            }

            generateMipmaps();
            createImageView();
            createSampler();
    }

    Texture::~Texture() {
        sampler.clear();
        sampler = nullptr;

        imageView.clear();
        imageView = nullptr;
        
        image.clear();
        image = nullptr;

        memory.clear();
        memory = nullptr;
    }

    std::pair<vk::raii::Image, vk::raii::DeviceMemory> Texture::createImage(vk::ImageCreateInfo &imageInfo, vk::MemoryPropertyFlags memoryProperties) {
        Core&   core                     = Core::getInstance();
        const vk::raii::Device& device   = core.getDevice();

        vk::raii::Image image = vk::raii::Image(device, imageInfo);

        vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo {
            .allocationSize   = memRequirements.size,
            .memoryTypeIndex  = core.findMemoryType(memRequirements.memoryTypeBits, memoryProperties)
        };

        vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(device, allocInfo);
        image.bindMemory(imageMemory, 0);

        return { std::move(image), std::move(imageMemory) };
    }

    void Texture::createImage() {
        vk::ImageCreateInfo imageInfo {
            .imageType         = vk::ImageType::e2D,
            .format            = format,
            .extent            = {width, height, 1},
            .mipLevels         = mipCount,
            .arrayLayers       = 1,
            .samples           = vk::SampleCountFlagBits::e1,
            .tiling            = tiling,
            .usage             = usageFlags,
            .sharingMode       = vk::SharingMode::eExclusive // fixing in exclusive mode for now
        };

        std::tie(image, memory) = createImage(imageInfo, vk::MemoryPropertyFlagBits::eDeviceLocal);
    }

    vk::raii::ImageView Texture::createImageView(const vk::Image &image, vk::Format format, vk::ImageAspectFlags aspectMaskFlags, uint32_t mipLevels) {
        vk::ImageViewCreateInfo viewInfo {
        .image            = image,
        .viewType         = vk::ImageViewType::e2D,
        .format           = format,
        .subresourceRange = {.aspectMask = aspectMaskFlags, .baseMipLevel = 0, .levelCount = mipLevels, .baseArrayLayer = 0, .layerCount = 1}};

        return vk::raii::ImageView(Core::getInstance().getDevice(), viewInfo);
    }

    void Texture::createImageView() {    
        imageView = createImageView(*image, format, aspectFlags, mipCount);
    }

    void Texture::createSampler() {
        vk::PhysicalDeviceProperties properties = Core::getInstance().getPhysicalDevice().getProperties();

        vk::SamplerCreateInfo samplerInfo {
            .magFilter                  = vk::Filter::eLinear,
            .minFilter                  = vk::Filter::eLinear,
            .mipmapMode                 = vk::SamplerMipmapMode::eLinear,
            .addressModeU               = vk::SamplerAddressMode::eRepeat,
            .addressModeV               = vk::SamplerAddressMode::eRepeat,
            .addressModeW               = vk::SamplerAddressMode::eRepeat,
            .mipLodBias                 = 0.0f,
            .anisotropyEnable           = vk::True,
            .maxAnisotropy              = properties.limits.maxSamplerAnisotropy,
            .compareEnable              = vk::False,
            .compareOp                  = vk::CompareOp::eAlways,
            .minLod                     = 0,
            .maxLod                     = vk::LodClampNone,
            .borderColor                = vk::BorderColor::eIntOpaqueBlack,
            .unnormalizedCoordinates    = vk::False
        };

        sampler = vk::raii::Sampler(Core::getInstance().getDevice(), samplerInfo);
    }

    void Texture::generateMipmaps() {
        // Check if image format supports linear blit-ing
        vk::FormatProperties formatProperties = Core::getInstance().getPhysicalDevice().getFormatProperties(format);

        if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) 
            throw std::runtime_error("texture image format does not support linear blitting!");
        
        CommandBuffer commandBuffer = CommandBuffer::singleTimeGraphicsCommand();

        vk::ImageMemoryBarrier2 barrier = {
            .srcStageMask           = vk::PipelineStageFlagBits2::eTransfer, .srcAccessMask          = vk::AccessFlagBits2::eTransferWrite,
            .dstStageMask           = vk::PipelineStageFlagBits2::eTransfer, .dstAccessMask          = vk::AccessFlagBits2::eTransferRead,
            .oldLayout              = vk::ImageLayout::eTransferDstOptimal,  .newLayout              = vk::ImageLayout::eTransferSrcOptimal,
            .srcQueueFamilyIndex    = vk::QueueFamilyIgnored,                .dstQueueFamilyIndex    = vk::QueueFamilyIgnored,
            .image                  = image,
            .subresourceRange       = {
                .aspectMask     = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1
            }
        };

        vk::DependencyInfo dependencyInfo = {
            .dependencyFlags         = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &barrier
        };

        int32_t mipWidth  = width;
        int32_t mipHeight = height;

        for (uint32_t i = 1; i < mipCount; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout       = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout       = vk::ImageLayout::eTransferSrcOptimal;
            barrier.srcAccessMask   = vk::AccessFlagBits2::eTransferWrite;
            barrier.dstAccessMask   = vk::AccessFlagBits2::eTransferRead;
            barrier.dstStageMask    = vk::PipelineStageFlagBits2::eTransfer;

            commandBuffer.addBarrier(dependencyInfo);

            vk::ArrayWrapper1D<vk::Offset3D, 2> srcOffsets {{{
                {0, 0, 0}, {mipWidth, mipHeight, 1}
            }}}; 
            
            vk::ArrayWrapper1D<vk::Offset3D, 2> dstOffsets {{{
                {0, 0, 0}, {mipWidth > 1 ? (mipWidth >> 1) : 1, mipHeight > 1 ? (mipHeight >> 1) : 1, 1}
            }}};
            
            vk::ImageBlit2 imageBlit = { 
                .srcSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i - 1, .baseArrayLayer = 0, .layerCount = 1 }, 
                .srcOffsets = srcOffsets,
                .dstSubresource = { .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i, .baseArrayLayer = 0, .layerCount = 1 }, 
                .dstOffsets = dstOffsets
            };

            vk::BlitImageInfo2 blitInfo {
                .srcImage    = image, .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
                .dstImage    = image, .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
                .regionCount = 1,
                .pRegions    = &imageBlit,
                .filter      = vk::Filter::eLinear
            };
            
            commandBuffer.blit(blitInfo);

            barrier.oldLayout       = vk::ImageLayout::eTransferSrcOptimal;
            barrier.newLayout       = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask   = vk::AccessFlagBits2::eTransferRead;
            barrier.dstAccessMask   = vk::AccessFlagBits2::eShaderRead;
            barrier.dstStageMask    = vk::PipelineStageFlagBits2::eFragmentShader;

            commandBuffer.addBarrier(dependencyInfo);

            if (mipWidth  > 1) mipWidth  >>= 1;
            if (mipHeight > 1) mipHeight >>= 1;
        }

        barrier.subresourceRange.baseMipLevel = mipCount - 1;
        barrier.oldLayout       = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout       = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask   = vk::AccessFlagBits2::eTransferWrite;
        barrier.dstAccessMask   = vk::AccessFlagBits2::eShaderRead;
        barrier.dstStageMask    = vk::PipelineStageFlagBits2::eFragmentShader;

        commandBuffer.addBarrier(dependencyInfo);
        commandBuffer.dispatch();
    }


    void Texture::copyBufferToImage(const CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer) {
        vk::BufferImageCopy2 region {
            .bufferOffset       = 0,
            .bufferRowLength    = 0,
            .imageSubresource   = { .aspectMask = aspectFlags, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
            .imageOffset        = { 0, 0, 0 },
            .imageExtent        = { width, height, 1}
        };

        vk::CopyBufferToImageInfo2 copyInfo {
            .srcBuffer       = buffer,
            .dstImage        = image,
            .dstImageLayout  = vk::ImageLayout::eTransferDstOptimal,
            .regionCount     = 1,
            .pRegions        = &region
        };

        commandBuffer.copyBufferToImage(copyInfo);
    }

    void Texture::updateImageLayout(const CommandBuffer &commandBuffer, const vk::ImageMemoryBarrier2 &barrier) {
        vk::DependencyInfo dependencyInfo = {
            .dependencyFlags         = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &barrier
        };

        commandBuffer.addBarrier(dependencyInfo);
    }

    void Texture::updateImageLayout(const CommandBuffer &commandBuffer, vk::ImageLayout targetLayout, vk::AccessFlags2 targetAccess, vk::PipelineStageFlags2 targetStage, uint32_t targetQueue) {
        vk::ImageMemoryBarrier2 barrier {
            .srcStageMask           = this->currentStage,       .srcAccessMask          = this->currentAccess,
            .dstStageMask           = targetStage,              .dstAccessMask          = targetAccess,
            .oldLayout              = this->currentLayout,      .newLayout              = targetLayout,
            .srcQueueFamilyIndex    = vk::QueueFamilyIgnored,   .dstQueueFamilyIndex    = vk::QueueFamilyIgnored,
            .image                  = image,
            .subresourceRange       = { .aspectMask = aspectFlags, .baseMipLevel = 0, .levelCount = (uint32_t)mipCount, .baseArrayLayer = 0, .layerCount = 1 }
        };

        // If image is already owned by a queue and we have a different target queue,
        // then we need to make a queue ownership release first.
        // TODO: This is not needed if vk::SharingMode::eConcurrent is used.
        if ((currentQueueIndex != vk::QueueFamilyIgnored) && (currentQueueIndex != targetQueue)) {
            barrier.srcQueueFamilyIndex = currentQueueIndex;
            barrier.dstQueueFamilyIndex = targetQueue;

            updateImageLayout(commandBuffer, barrier);
        }
        
        updateImageLayout(commandBuffer, barrier);

        currentQueueIndex = targetQueue;
    }

    const vk::raii::Image& Texture::getImage() const { return image; }
    const vk::raii::ImageView& Texture::getImageView() const { return imageView; }
    const vk::raii::Sampler& Texture::getSampler() const { return sampler; }
}