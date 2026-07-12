#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include <unordered_map>

#include <engine/vmaAllocator.hpp>
#include <vk_mem_alloc.h>

#include <engine/models.hpp>
using namespace Engine::Models;

#include <graphics/core.hpp>
using Graphics::Core;

#include <utils/slotmap.hpp>
using Utils::SlotKey;
using Utils::SlotMap;

namespace Engine {
    class ResourceManager {
        public:
            static ResourceManager& getInstance();

            ResourceManager(const ResourceManager&) = delete;
            ResourceManager(ResourceManager&&) noexcept = delete;

            ResourceManager& operator=(const ResourceManager&) = delete;
            ResourceManager& operator=(ResourceManager&&) noexcept = delete;

            const bool isInCache(const std::string& name) const;

            ~ResourceManager();
        private:
            ResourceManager();

            VmaAllocator allocator;

            template <typename Resource, typename Info>
            const SlotKey& load(const std::string& name, const Info& info = {});

            template <typename Resource>
            void release(const SlotKey& key);

            template <typename Resource>
            const Resource& get(const SlotKey& key);

            std::unordered_map<std::string, SlotKey> cache;

            struct Pools {
                SlotMap<Mesh>    meshes;
                SlotMap<Texture> textures;
                SlotMap<Shader>  shaders;
            } pools;
    };
}

#include <engine/resourceManager.tpp>

#endif