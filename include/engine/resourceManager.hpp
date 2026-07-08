#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include <graphics/core.hpp>

#include <engine/vmaAllocator.hpp>
#include <vk_mem_alloc.h>

using Graphics::Core;

namespace Engine {
    class ResourceManager {
        public:
            static ResourceManager& getInstance();

            ResourceManager(const ResourceManager&) = delete;
            ResourceManager(ResourceManager&&) noexcept = delete;

            ResourceManager& operator=(const ResourceManager&) = delete;
            ResourceManager& operator=(ResourceManager&&) noexcept = delete;

            ~ResourceManager();
        private:
            ResourceManager();

            VmaAllocator allocator;
    };
}

#endif