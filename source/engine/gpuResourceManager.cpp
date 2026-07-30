#include <engine/gpuAllocator.hpp>
#include <vk_mem_alloc.h>

#include <engine/gpuResourceManager.hpp>

#include <graphics/core.hpp>
#include <graphics/commandBuffer.hpp>
using Graphics::Core;
using Graphics::CommandBuffer;


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

    void GPUResourceManager::allocateBuffer(VkImage& image, VmaAllocation& allocation, const VkDeviceSize size, const VkImageCreateInfo& createInfo, const uint32_t targetQueIndex) {
        VmaAllocationCreateInfo allocationCreateInfo {
            .usage = VMA_MEMORY_USAGE_AUTO
        };

        vmaCreateImage(allocator, &createInfo, &allocationCreateInfo, &image, &allocation, nullptr);
    }

    void GPUResourceManager::allocateBuffer(VkBuffer& buffer, VmaAllocation& allocation, const VkDeviceSize size, const VkBufferUsageFlags usage, const uint32_t targetQueueIndex) {
        VkBufferCreateInfo bufferInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size  = size,
            .usage = usage
        };        

        if (Graphics::Core::getInstance().hasDedicatedTransferQueue() && targetQueueIndex != Graphics::Core::getInstance().getTransferQueueIndex()) {
            std::array transferAndGraphicsQueues { 
                Graphics::Core::getInstance().getGraphicsQueueIndex(),
                Graphics::Core::getInstance().getTransferQueueIndex()
            };

            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = 2;
            bufferInfo.pQueueFamilyIndices = &transferAndGraphicsQueues[0];
        }

        VmaAllocationCreateInfo allocationCreateInfo {
            .usage = VMA_MEMORY_USAGE_AUTO
        };

        VmaAllocationInfo allocationInfo;

        vmaCreateBuffer(allocator, &bufferInfo, &allocationCreateInfo, &buffer, &allocation, &allocationInfo);
    }

    void GPUResourceManager::uploadData(VkImage& image, VmaAllocation& allocation, const void* src, const VkDeviceSize size, const VkImageCreateInfo& createInfo, const vk::ImageAspectFlagBits imageAspect, const uint32_t targetQueueIndex) {
        allocateBuffer(image, allocation, size, createInfo, targetQueueIndex);

        vk::ImageMemoryBarrier2 barrier {
            .srcStageMask           = vk::PipelineStageFlagBits2::eTransfer,    .srcAccessMask          = vk::AccessFlagBits2::eNone,
            .dstStageMask           = vk::PipelineStageFlagBits2::eTransfer,    .dstAccessMask          = vk::AccessFlagBits2::eTransferWrite,
            .oldLayout              = vk::ImageLayout::eUndefined,              .newLayout              = vk::ImageLayout::eTransferDstOptimal,
            .srcQueueFamilyIndex    = vk::QueueFamilyIgnored,                   .dstQueueFamilyIndex    = vk::QueueFamilyIgnored,
            .image                  = image,
            .subresourceRange       = { .aspectMask = imageAspect, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
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
            .imageSubresource   = { .aspectMask = imageAspect, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
            .imageOffset        = { 0, 0, 0 },
            .imageExtent        = { createInfo.extent.width, createInfo.extent.height, createInfo.extent.depth }
        };

        vk::CopyBufferToImageInfo2 copyInfo {
            .srcBuffer       = stagingBuffer.buffer,
            .dstImage        = image,
            .dstImageLayout  = vk::ImageLayout::eTransferDstOptimal,
            .regionCount     = 1,
            .pRegions        = &region
        };

        VkDeviceSize written = 0;
        while (written < size) {   
            VkDeviceSize chunkSize = std::min(stagingBuffer.capacity, size - written);
            
            std::cout << "Transfering image... " << written << "/" << size << '\n';

            memcpy(stagingBuffer.allocInfo.pMappedData, (const char*)src + written, size);

            region.bufferOffset = written;
            CommandBuffer::singleTimeTransfer().copyBufferToImage(copyInfo).submit();

            written += chunkSize;
            std::cout << "Transferred chunk: " << chunkSize << '\n';
        }
        
        barrier.dstStageMask  = vk::PipelineStageFlagBits2::eFragmentShader;
        barrier.dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead;
        barrier.newLayout     = vk::ImageLayout::eShaderReadOnlyOptimal;

        if (Core::getInstance().getTransferQueueIndex() != targetQueueIndex) {
            barrier.srcQueueFamilyIndex = Core::getInstance().getTransferQueueIndex();
            barrier.dstQueueFamilyIndex = targetQueueIndex;

            CommandBuffer::singleTimeTransfer().addImageBarrier(dependencyInfo).submit();
        }

        CommandBuffer::singleTimeGraphics().addImageBarrier(dependencyInfo).submit();     
    }

    void GPUResourceManager::uploadData(VkBuffer& buffer, VmaAllocation& allocation, const void* src, const VkDeviceSize size, const VkBufferUsageFlags usage, const uint32_t targetQueueIndex) {
        allocateBuffer(buffer, allocation, size, usage, targetQueueIndex);

        VkDeviceSize written = 0;
        
        while (written < size) {
            
            VkDeviceSize chunkSize = std::min(stagingBuffer.capacity, size - written);
            
            std::cout << "Writting GPU memory: " << written << "/" << size << '\n';

            memcpy(stagingBuffer.allocInfo.pMappedData, (const char*)src + written, (size_t)chunkSize);

            CommandBuffer::singleTimeTransfer().copyBuffer(stagingBuffer.buffer, buffer, vk::BufferCopy(0, written, chunkSize)).submit();

            written += chunkSize;
        }
    }

    void GPUResourceManager::updateResourceState(const SlotKey& key, const ImageState& state) {
        imageStates[key] = state;
    }

    void GPUResourceManager::registerResourceState(const SlotKey& key, const ImageState& initialState) {
        imageStates.emplace(key, std::move(initialState));
    }

    void GPUResourceManager::unregisterResourceState(const SlotKey& key) {
        imageStates.erase(key);
    }
}