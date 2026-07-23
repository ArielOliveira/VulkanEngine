#ifndef ENGINE_RESOURCE_MANAGER_HPP
#define ENGINE_RESOURCE_MANAGER_HPP

#include <stack>
#include <vector>
#include <unordered_map>
#include <typeindex>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/types.hpp>

#include <engine/resourceHandle.hpp>
#include <engine/resources.hpp>

#include <utils/slotmap.hpp>
#include <utils/passKey.hpp>

using Utils::SlotKey;
using Utils::SlotMap;
using Utils::ResourceSlot;
using Utils::PassKey;

using fastgltf::Asset;

namespace Engine {
    using namespace Engine::Resources;

    class ResourceManager {
        public:
            static ResourceManager& getInstance();

            ResourceManager(const ResourceManager&) = delete;
            ResourceManager(ResourceManager&&) noexcept = delete;

            ResourceManager& operator=(const ResourceManager&) = delete;
            ResourceManager& operator=(ResourceManager&&) noexcept = delete;

            ~ResourceManager();

            template <typename Resource>
            const ResourceHandle<Resource> load(const std::string& path);

            template <typename Resource>
            const ResourceHandle<Resource> load(const std::string& sourceName, const Asset& asset, const size_t resourceIndex);

            template <typename Resource>
            void acquire(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&);

            template <typename Resource>
            void release(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&);

            template <typename Resource>
            const Resource& get(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const;
        
        private:
            template <typename... ResourceType>
            struct Pools {
                std::tuple<SlotMap<ResourceType, ResourceSlot>...> pools;

                template<typename T>
                SlotMap<T, ResourceSlot>& get() {
                    return std::get<SlotMap<T, ResourceSlot>>(pools);
                }

                template<typename T>
                const SlotMap<T, ResourceSlot>& get() const {
                    return std::get<SlotMap<T, ResourceSlot>>(pools);
                }
            };

            using ResourcesPool = Pools<Model, Scene, Mesh, Material, Shader, Texture, Animation>;

            ResourceManager();

            template <typename Resource>
            void destroy(const std::string& name, const SlotKey& key);

            SlotKey* const findKeyByName(const std::type_index& typeId, const std::string& name);

            ResourcesPool pools;
            
            std::unordered_map<std::type_index, std::unordered_map<std::string, SlotKey>> cache;
    };
}

#endif