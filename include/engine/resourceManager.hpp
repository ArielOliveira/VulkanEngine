#ifndef ENGINE_RESOURCE_MANAGER_HPP
#define ENGINE_RESOURCE_MANAGER_HPP

#include <unordered_map>
#include <typeindex>

#include <engine/resourceTypes.hpp>
#include <engine/resources.hpp>

#include <utils/slotmap.hpp>
#include <utils/passKey.hpp>

using namespace Engine::Resources;
using namespace Engine::Resources::Descriptors;

using Utils::SlotKey;
using Utils::SlotMap;
using Utils::ResourceSlot;
using Utils::PassKey;

namespace Engine {
    class ResourceManager {
        public:
            static ResourceManager& getInstance();

            ResourceManager(const ResourceManager&) = delete;
            ResourceManager(ResourceManager&&) noexcept = delete;

            ResourceManager& operator=(const ResourceManager&) = delete;
            ResourceManager& operator=(ResourceManager&&) noexcept = delete;

            ~ResourceManager();

            template <typename Resource>
            const ResourceHandle<Resource>& load(const std::string& path);

            template <typename Resource>
            void acquire(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&);

            template <typename Resource>
            void release(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&);
        private:
            template <typename... ResourceType>
            struct Pools {
                std::tuple<SlotMap<ResourceType, ResourceSlot>...> pools;

                template<typename T>
                SlotMap<T, ResourceSlot>& get() {
                    return std::get<SlotMap<T, ResourceSlot>>(pools);
                }
            };

            using ResourcesPool = Pools<Model, Scene, Mesh, Material, Shader, Texture, Animation>;

            ResourceManager();

            template <typename Resource>
            void destroy(const std::string& name, const SlotKey& key);

            SlotKey* const findKeyByName(const std::type_index& typeId, const std::string& name);

            
            VmaAllocator allocator;
            ResourcesPool pools;
            
            std::unordered_map<std::type_index, std::unordered_map<std::string, SlotKey>> cache;
    };
}

#endif