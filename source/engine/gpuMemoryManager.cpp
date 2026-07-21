#include <engine/gpuAllocator.hpp>
#include <vk_mem_alloc.h>

#include <engine/gpuMemoryManager.hpp>

#include <graphics/core.hpp>
#include <graphics/commandBuffer.hpp>
using Graphics::Core;
using Graphics::CommandBuffer;

namespace Engine {
    GPUMemoryManager& GPUMemoryManager::getInstance() {
        static GPUMemoryManager instance;

        return instance;
    }

    GPUMemoryManager::GPUMemoryManager() {
        intializeAllocator();
        createStagingBuffer();
    }

    GPUMemoryManager::~GPUMemoryManager() {
        vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
        vmaDestroyAllocator(allocator);
    }

    void GPUMemoryManager::intializeAllocator() {
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

    void GPUMemoryManager::createStagingBuffer() {
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

    void GPUMemoryManager::releaseBuffer(VkBuffer& buffer, VmaAllocation& allocation) {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }

    void GPUMemoryManager::allocateBuffer(VkBuffer& buffer, VmaAllocation& allocation, VkDeviceSize size, VkBufferUsageFlags usage) {
        VkBufferCreateInfo bufferInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size  = size,
            .usage = usage
        };        

        if (Graphics::Core::getInstance().hasDedicatedTransferQueue()) {
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

    void GPUMemoryManager::uploadDataToGPU(VkBuffer& buffer, VmaAllocation& allocation, const void* src, VkDeviceSize size, VkBufferUsageFlags usage) {
        allocateBuffer(buffer, allocation, size, usage);

        VkDeviceSize written = 0;

        while (written < size) {
            
            VkDeviceSize chunkSize = std::min(stagingBuffer.capacity, size - written);
            
            std::cout << "Writting GPU memory: " << written << "/" << size << '\n';

            memcpy(stagingBuffer.allocInfo.pMappedData, (const char*)src + written, (size_t)chunkSize);

            CommandBuffer::singleTimeTransfer().copyBuffer(stagingBuffer.buffer, buffer, vk::BufferCopy(0, written, chunkSize)).submit();

            written += chunkSize;
        }
    }
}