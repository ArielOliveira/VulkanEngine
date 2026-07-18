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

#include <graphics/models.hpp>

#include <engine/fileParser.hpp>
#include <engine/gpuMemoryManager.hpp>

#include <utils/slotmap.hpp>
#include <utils/passKey.hpp>

using Utils::SlotKey;
using Utils::SlotMap;
using Utils::ResourceSlot;
using Utils::PassKey;

using fastgltf::Asset;

namespace Engine {
template <typename T>
    class ResourceHandle {
        public:
            ResourceHandle() = delete;
            ResourceHandle(SlotKey key) noexcept;
            ResourceHandle(std::nullptr_t) noexcept;
            ResourceHandle(const ResourceHandle&) noexcept;
            ResourceHandle(ResourceHandle&& rhs) noexcept;

            ~ResourceHandle() noexcept;

            const ResourceHandle& operator=(std::nullptr_t) noexcept;
            const ResourceHandle& operator=(const ResourceHandle& rhs) = delete;
            ResourceHandle& operator=(ResourceHandle&& rhs) noexcept;

            const T& data() const;

        private:
            SlotKey key = { ~0U, ~0U };
    };
}

namespace Engine::Resources {
    struct Shader {
        std::string name;

        VkShaderModule shaderModule;
        VkShaderStageFlagBits stage;
    };

    struct Animation {
        std::string name;
    };

    struct Texture {
        std::string name;

        VmaAllocation allocation;

        VkImage image;
        VkImageView view;
        VkSampler sampler;

        uint32_t width;
        uint32_t height;
        uint32_t channels;
    };

    struct Material {
        std::string name;
    };

    struct Mesh {
        std::string name;

        VmaAllocation vertexAllocation;
        VmaAllocation indexAllocation;

        VkBuffer vertexBuffer;
        VkBuffer indexBuffer;
        
        VkIndexType indexType;

        uint32_t vertexCount;
        uint32_t indexCount;

        // Submesh data
        vector<uint32_t> smIndexOffset;
        vector<uint32_t> smIndexCount;
        vector<uint32_t> smVertexOffset;
        vector<ResourceHandle<Material>>  smMaterials;
    };

    struct Scene {
        std::string         name;

        vector<std::string> nodeNames;
        vector<uint32_t>    hierarchy;
        vector<glm::mat4x4> transforms;
        
        vector<std::pair<ResourceHandle<Mesh>, uint32_t>>      meshes;
        vector<std::pair<ResourceHandle<Animation>, uint32_t>> animations;
    };

    struct Model {
        std::string                   name;
        
        vector<ResourceHandle<Scene>> scenes;
    };
}

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

            const ResourceHandle<Model> load(const std::string& path);

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