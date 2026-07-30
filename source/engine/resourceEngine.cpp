#include <engine/resourceEngine.hpp>

namespace Engine {
    ResourceManager& ResourceManager::getInstance() {
        static ResourceManager instance;

        return instance;
    }

    ResourceManager::ResourceManager() { 
        // Ensures order of initialization 
        // GPU memory allocator is guaranteed to be initialized by the time 
        // it is needed
        GPUResourceManager::getInstance(); 
    }

    ResourceManager::~ResourceManager() {}

    const ResourceHandle<Image> ResourceManager::createImage(const std::string& name, const void* data, const VkDeviceSize size, const VkImageCreateInfo& imageCreateInfo, VkImageViewCreateInfo& viewCreateInfo, const uint32_t channels, const uint32_t targetQueue) {
        VkImage image;
        VmaAllocation allocation;
        GPUResourceManager::getInstance().uploadData(
                image, allocation, 
                data, size,
                imageCreateInfo, 
                vk::ImageAspectFlagBits::eColor,
                Core::getInstance().getGraphicsQueueIndex());

        viewCreateInfo.image = image;
        
        VkImageView view;
        vkCreateImageView(*Core::getInstance().getDevice(), &viewCreateInfo, nullptr, &view);

        SlotKey key = pools.get<Image>().emplace(std::move( Image{
            .name       = name,
            .allocation = allocation,
            .image      = image,
            .view       = view,
            .width      = imageCreateInfo.extent.width,
            .height     = imageCreateInfo.extent.height,
            .channels   = channels,
            .mipCount   = 1,
        }));

        GPUResourceManager::getInstance().registerResourceState(key, {});
        
        cache[std::type_index(typeid(Image))][name] = key;

        return std::move(ResourceHandle<Image>(key));
    }

    const ResourceHandle<Image> ResourceManager::createImage(const std::string& name, const VkImageCreateInfo& imageCreateInfo, const VkImageViewCreateInfo& viewCreateInfo, const uint32_t channels, const uint32_t targetQueue) {
        
    }

    SlotKey* const ResourceManager::findKeyByName(const std::type_index& typeId, const std::string& name) { 
        auto it = cache.find(typeId);
        
        if (it == cache.end()) return nullptr;

        try {
            return &it->second.at(name);
        } catch(std::exception e) {
            return  nullptr;
        }
    }
}