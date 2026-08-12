#include <engine/gpuAllocator.hpp>
#include <vk_mem_alloc.h>

#include <engine/gpuResourceManager.hpp>

#include <graphics/core.hpp>

using Graphics::Core;


namespace Engine {
    GPUResourceManager& GPUResourceManager::getInstance() {
        static GPUResourceManager instance;

        return instance;
    }

    GPUResourceManager::GPUResourceManager() {
        intializeAllocator();
        createStagingBuffer();
    }

    GPUResourceManager::~GPUResourceManager() {
        vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
        vmaDestroyAllocator(allocator);
    }

    void GPUResourceManager::intializeAllocator() {
        VmaVulkanFunctions vulkanFunctions {
            .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr   = &vkGetDeviceProcAddr
        };

        VmaAllocatorCreateInfo allocatorCreateInfo {
            .flags             = {},
            .physicalDevice    = *Core::getInstance().getPhysicalDevice(),
            .device            = *Core::getInstance().getDevice(),
            .pVulkanFunctions  = &vulkanFunctions,
            .instance          = *Core::getInstance().getVulkanInstance(),
            .vulkanApiVersion  =  Core::getInstance().getPhysicalDevice().getProperties().apiVersion,
        };

        vmaCreateAllocator(&allocatorCreateInfo, &allocator);
    }

    void GPUResourceManager::createStagingBuffer() {
        stagingBuffer.capacity = STAGING_BUFFER_SIZE;

        VkBufferCreateInfo bufferInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size  = STAGING_BUFFER_SIZE,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        };

        VmaAllocationCreateInfo allocationCreateInfo = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };

        vmaCreateBuffer(allocator, &bufferInfo, &allocationCreateInfo, &stagingBuffer.buffer, &stagingBuffer.allocation, &stagingBuffer.allocInfo);
    }

    void GPUResourceManager::releaseBuffer(VkImage& image, VmaAllocation& allocation) {
        vmaDestroyImage(allocator, image, allocation);
    }

    void GPUResourceManager::releaseBuffer(VkBuffer& buffer, VmaAllocation& allocation) {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }

    void GPUResourceManager::allocateBuffer(VkImage& image, VmaAllocation& allocation, const VkImageCreateInfo& createInfo) {
        VmaAllocationCreateInfo allocationCreateInfo {
            .usage = VMA_MEMORY_USAGE_AUTO
        };

        vmaCreateImage(allocator, &createInfo, &allocationCreateInfo, &image, &allocation, nullptr);
    }

    void GPUResourceManager::allocateBuffer(VkBuffer& buffer, VmaAllocation& allocation, const VkBufferCreateInfo& createInfo) {
        VmaAllocationCreateInfo allocationCreateInfo {
            .usage = VMA_MEMORY_USAGE_AUTO
        };

        VmaAllocationInfo allocationInfo;

        vmaCreateBuffer(allocator, &createInfo, &allocationCreateInfo, &buffer, &allocation, &allocationInfo);
    }

    const ImageState GPUResourceManager::uploadData(VkImage& image, VmaAllocation& allocation, const VkImageCreateInfo& createInfo) {
        allocateBuffer(image, allocation, createInfo);

        return {
            std::vector<vk::PipelineStageFlags2>(createInfo.mipLevels, vk::PipelineStageFlagBits2::eTransfer),
            std::vector<vk::AccessFlags2>(createInfo.mipLevels, vk::AccessFlagBits2::eTransferWrite),
            std::vector<vk::ImageLayout>(createInfo.mipLevels, vk::ImageLayout::eTransferDstOptimal),
            createInfo.sharingMode == VkSharingMode::VK_SHARING_MODE_CONCURRENT ? 
                 VK_QUEUE_FAMILY_IGNORED :
                 Core::getInstance().getTransferQueueIndex()
        };
    }

    const ImageState GPUResourceManager::uploadData(VkImage& image, VmaAllocation& allocation, const std::byte* src, const VkDeviceSize size, const ImageLayout& imageLayout, const VkImageCreateInfo& createInfo, const VkImageAspectFlags imageAspect) {
        allocateBuffer(image, allocation, createInfo);

        vk::ImageMemoryBarrier2 barrier {
            .srcStageMask           = vk::PipelineStageFlagBits2::eTransfer,    .srcAccessMask          = vk::AccessFlagBits2::eNone,
            .dstStageMask           = vk::PipelineStageFlagBits2::eTransfer,    .dstAccessMask          = vk::AccessFlagBits2::eTransferWrite,
            .oldLayout              = vk::ImageLayout::eUndefined,              .newLayout              = vk::ImageLayout::eTransferDstOptimal,
            .srcQueueFamilyIndex    = vk::QueueFamilyIgnored,                   .dstQueueFamilyIndex    = vk::QueueFamilyIgnored,
            .image                  = image,
            .subresourceRange       = { .aspectMask = static_cast<vk::ImageAspectFlags>(imageAspect), .baseMipLevel = 0, .levelCount = static_cast<uint32_t>(imageLayout.mips.size()), .baseArrayLayer = 0, .layerCount = imageLayout.layers }
        };

        vk::DependencyInfo dependencyInfo = {
            .dependencyFlags         = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &barrier
        };

        CommandBuffer::singleTimeTransfer().addImageBarrier(dependencyInfo).submit();

        vk::BufferImageCopy2 region {
            .bufferOffset       = 0,
            .bufferRowLength    = 0,
            .bufferImageHeight  = 0,
            .imageSubresource   = { .aspectMask = static_cast<vk::ImageAspectFlags>(imageAspect), .mipLevel = 0, .baseArrayLayer = 0, .layerCount = imageLayout.layers},
            .imageOffset        = { 0, 0, 0 },
            .imageExtent        = { imageLayout.mips[0].width, imageLayout.mips[0].height, imageLayout.mips[0].depth }
        };

        vk::CopyBufferToImageInfo2 copyInfo {
            .srcBuffer       = stagingBuffer.buffer,
            .dstImage        = image,
            .dstImageLayout  = vk::ImageLayout::eTransferDstOptimal,
            .regionCount     = 1,
            .pRegions        = &region
        };

        for (size_t i = 0; i < imageLayout.mips.size(); i++) {
            auto mipLayout = imageLayout.mips[i];

            region.imageSubresource.mipLevel = i;
            
            size_t storageRows   = (mipLayout.height + imageLayout.blockHeight - 1) / imageLayout.blockHeight;
            
            size_t rowsPerChunk  = std::max(1ULL, (static_cast<size_t>(stagingBuffer.capacity) / mipLayout.storageRowPitch));
            size_t rowsWritten   = 0;
            
            while(rowsWritten < storageRows) {
                size_t rowsToWrite = std::min(rowsPerChunk, storageRows - rowsWritten); 
                size_t bytes       = (size_t)rowsToWrite * (size_t)mipLayout.storageRowPitch;   

                memcpy(stagingBuffer.allocInfo.pMappedData, (src + mipLayout.offset) + (bytes * rowsWritten), bytes);

                uint32_t currentY      = rowsWritten * imageLayout.blockHeight;
                uint32_t rowsThisChunk = std::min(static_cast<uint32_t>(rowsToWrite) * imageLayout.blockHeight, mipLayout.height - currentY);

                region.imageOffset.y = currentY;
                region.setImageExtent({ mipLayout.width, rowsThisChunk, 1 });

                CommandBuffer::singleTimeTransfer().copyBufferToImage(copyInfo).submit();

                rowsWritten += rowsToWrite;
            }
        }

        return {
            std::vector<vk::PipelineStageFlags2>(imageLayout.mips.size(), vk::PipelineStageFlagBits2::eTransfer),
            std::vector<vk::AccessFlags2>(imageLayout.mips.size(), vk::AccessFlagBits2::eTransferWrite),
            std::vector<vk::ImageLayout>(imageLayout.mips.size(), vk::ImageLayout::eTransferDstOptimal),
            createInfo.sharingMode == VkSharingMode::VK_SHARING_MODE_CONCURRENT ? 
                 VK_QUEUE_FAMILY_IGNORED :
                 Core::getInstance().getTransferQueueIndex() 
        };
        
      /*barrier.dstStageMask  = vk::PipelineStageFlagBits2::eFragmentShader;
        barrier.dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead;
        barrier.newLayout     = vk::ImageLayout::eShaderReadOnlyOptimal;

        if (Core::getInstance().getTransferQueueIndex() != targetQueueIndex) {
            barrier.srcQueueFamilyIndex = Core::getInstance().getTransferQueueIndex();
            barrier.dstQueueFamilyIndex = targetQueueIndex;

            CommandBuffer::singleTimeTransfer().addImageBarrier(dependencyInfo).submit();
        }

        CommandBuffer::singleTimeGraphics().addImageBarrier(dependencyInfo).submit();*/
    }

    const BufferState GPUResourceManager::uploadData(VkBuffer& buffer, VmaAllocation& allocation, const std::byte* src, const VkBufferCreateInfo& createInfo) {
        allocateBuffer(buffer, allocation, createInfo);

        VkDeviceSize written = 0;
        
        while (written < createInfo.size) {
            VkDeviceSize chunkSize = std::min(stagingBuffer.capacity, createInfo.size - written);
            
            std::cout << "Writting GPU memory: " << written << "/" << createInfo.size << '\n';

            memcpy(stagingBuffer.allocInfo.pMappedData, src + written, (size_t)chunkSize);

            CommandBuffer::singleTimeTransfer().copyBuffer(stagingBuffer.buffer, buffer, vk::BufferCopy(0, written, chunkSize)).submit();

            written += chunkSize;
        }

        return { vk::PipelineStageFlagBits2::eTransfer, 
                 vk::AccessFlagBits2::eTransferWrite,
                 createInfo.sharingMode == VkSharingMode::VK_SHARING_MODE_CONCURRENT ? 
                 VK_QUEUE_FAMILY_IGNORED :
                 Core::getInstance().getTransferQueueIndex() };
    }
}