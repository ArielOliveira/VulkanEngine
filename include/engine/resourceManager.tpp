#include <engine/resourceManager.hpp>

namespace Engine {
    ResourceManager& ResourceManager::getInstance() {
        static ResourceManager instance;

        return instance;
    }

    ResourceManager::ResourceManager() {
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
            .vulkanApiVersion  = Core::getInstance().getPhysicalDevice().getProperties().apiVersion,
        };

        vmaCreateAllocator(&allocatorCreateInfo, &allocator);
    }

    ResourceManager::~ResourceManager() {
        vmaDestroyAllocator(allocator);
    }

    template<>
    const SlotKey& ResourceManager::load<Mesh, int>(const std::string& name, const int& info) {
        if (isInCache(name)) 
            throw std::runtime_error("Trying to load a cached resource!");

        return {};
    }

    const bool ResourceManager::isInCache(const std::string& name) const { return cache.contains(name); }
}