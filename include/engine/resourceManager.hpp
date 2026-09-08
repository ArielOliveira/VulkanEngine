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

#include <engine/fileParser.hpp>
#include <engine/resources.hpp>

#include <utils/slotmap.hpp>
#include <utils/passKey.hpp>

#include <graphics/commandBuffer.hpp>

using Utils::SlotMap;
using Utils::PassKey;

using Engine::Descriptors::TextureAsset;
using Graphics::CommandBuffer;

using fastgltf::Asset;

using namespace Engine::Resources;
using namespace Engine::Resources::Trackers;

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
            ResourceHandle<Resource> load(const std::string& path);

            template <typename Resource>
            ResourceHandle<Resource> load(const std::string& sourceName, const Asset& asset, const size_t resourceIndex);

            template <typename Resource>
            void acquire(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&);

            template <typename Resource>
            void release(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&);

            template <typename Resource>
            const bool contains(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const;

            template <typename Resource>
            const Resource& get(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const;

            template <typename Resource>
            const ResourceState& getState(const SlotKey& key, const PassKey<ResourceHandle<Resource>>&) const;
        
        private:
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