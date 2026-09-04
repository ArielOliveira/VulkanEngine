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

            const ResourceHandle<Image> createImage(const std::string& name, const TextureAsset& textureMetaData, const VkImageCreateInfo& imageCreateInfo, const VkImageAspectFlags imageAspect);
            const ResourceHandle<Image> createImage(const std::string& name, const VkImageCreateInfo& imageCreateInfo, const VkImageAspectFlags imageAspect); 

            void recreate(const ResourceHandle<Image>& handle, const VkImageCreateInfo& imageCreateInfo, const VkImageAspectFlags imageAspect, const PassKey<ResourceHandle<Image>>&);

            template <typename Resource>
            const ResourceHandle<Resource> load(const std::string& path);

            template <typename Resource>
            const ResourceHandle<Resource> load(const std::string& sourceName, const Asset& asset, const size_t resourceIndex);

            const ResourceHandle<Image> registerExternal(
                const std::string& name, const VkImage& image, const VkImageAspectFlags imageAspect, const VkFormat format,
                const uint32_t width, const uint32_t height, const uint32_t depth,
                const uint32_t mipCount = 1, const uint32_t layerCount = 1,
                const ImageState& state = { make_state_array({}), VK_QUEUE_FAMILY_IGNORED },
                const VmaAllocation& allocation = VK_NULL_HANDLE);

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
            template <typename... ResourceTypes>
            struct Pools {
                std::tuple<SlotMap<ResourceTypes, ResourceSlot>...> pools;

                template<typename T>
                SlotMap<T, ResourceSlot>& get() {
                    return std::get<SlotMap<T, ResourceSlot>>(pools);
                }

                template<typename T>
                const SlotMap<T, ResourceSlot>& get() const {
                    return std::get<SlotMap<T, ResourceSlot>>(pools);
                }
            };

            using ResourcesPool = Pools<Model, Scene, Mesh, Material, Shader, Texture, Image, Animation>;

            ResourceManager();

            template <typename Resource>
            void destroy(const std::string& name, const SlotKey& key);

            SlotKey* const findKeyByName(const std::type_index& typeId, const std::string& name);

            ResourcesPool pools;
            
            std::unordered_map<std::type_index, std::unordered_map<std::string, SlotKey>> cache;
    };
}

#endif