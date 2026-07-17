#ifndef ENGINE_RESOURCE_MANAGER_TPP
#define ENGINE_RESOURCE_MANAGER_TPP

#include <graphics/core.hpp>

#include <engine/fileParser.hpp>
#include <engine/resourceManager.hpp>
#include <engine/resourceHandle.hpp>
#include <engine/fileParser.hpp>

#include <engine/vmaAllocator.hpp>
#include <vk_mem_alloc.h>

using Graphics::Core;

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

    template <typename Resource>
    const ResourceHandle<Resource>& ResourceManager::load(const std::string& path) {
        assert(findKeyByName(std::type_index(typeid(Resource)), path) == nullptr);
    
        auto [parser, asset] = FileParser::openFile(path);
        
        ModelImportResult importResult = FileParser::parseModel(parser, asset.get());


       return nullptr;
    }

    template <typename Resource>
    void ResourceManager::acquire(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) {
        assert(key.generations == pools.get<Resource>().getSlot(key).generations);

        pools.get<Resource>().getSlot(key).refCount++;
    }

    template <typename Resource>
    void ResourceManager::release(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) {
        auto& pool = pools.get<Resource>();

        assert(key.generations == pool.getSlot(key).generations);

        if (--pool.getSlot(key).refCount == 0)
            pool.erase(key);
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

#endif