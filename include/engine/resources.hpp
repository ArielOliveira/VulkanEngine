#ifndef ENGINE_RESOURCES_HPP
#define ENGINE_RESOURCES_HPP

#include <cstdint>
#include <vector>
#include <string>

#include <glm/mat4x4.hpp>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

#include <engine/resourceHandle.hpp>

using std::vector;

namespace vk {
    enum class IndexType;
}

namespace Engine::Resources::GPU {
    struct Buffer {
        VkBuffer     buffer;
        VkDeviceSize size;
    };

    struct Image {
        VkImage              image;
        VkImageView          view;
        VkFormat             format;
        vk::ImageAspectFlags aspect;

        uint32_t width, height, depth;
        uint32_t mipCount, layers;
        uint32_t channels;
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

        ResourceHandle<GPU::Image> image;

        VkSampler sampler;
    };

    struct Material {
        std::string name;

        ResourceHandle<Texture> baseMap;
        ResourceHandle<Texture> normalMap = nullptr;
    };

    struct Mesh {
        std::string name;

        ResourceHandle<GPU::Buffer> vertexHandle;
        ResourceHandle<GPU::Buffer> indexHandle;
        
        vk::IndexType indexType;

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

    template <typename... ResourceTypes>
    struct Pools {
        std::tuple<Utils::SlotMap<ResourceTypes, Utils::Types::ResourceSlot>...> pools;

        template<typename T>
        Utils::SlotMap<T, Utils::Types::ResourceSlot>& get() {
            return std::get<Utils::SlotMap<T, Utils::Types::ResourceSlot>>(pools);
        }

        template<typename T>
        const Utils::SlotMap<T, Utils::Types::ResourceSlot>& get() const {
            return std::get<Utils::SlotMap<T, Utils::Types::ResourceSlot>>(pools);
        }
    };
}

namespace Engine::Resources::Trackers {
    constexpr uint8_t MAX_MIP_COUNT = 16;

    struct BufferState {
        vk::PipelineStageFlags2 currentStage;
        vk::AccessFlags2        currentAccess;

        uint32_t currentQueueFamily = ~0U;
    };

    struct MipSyncState {
        vk::PipelineStageFlags2 stage  = vk::PipelineStageFlagBits2::eNone;
        vk::AccessFlags2        access = vk::AccessFlagBits2::eNone;
        vk::ImageLayout         layout = vk::ImageLayout::eUndefined;
    };

    // Instead of a vector use a array maxed to
    // maybe 16 indices or use a small vector library
    struct ImageState {
        std::array<MipSyncState, MAX_MIP_COUNT> states;
        
        uint32_t currentQueueFamily = VK_QUEUE_FAMILY_IGNORED;
    };

    constexpr std::array<MipSyncState, MAX_MIP_COUNT> make_state_array(const MipSyncState& value) {
        std::array<MipSyncState, MAX_MIP_COUNT> stateArr{};

        for (int i = 0; i < MAX_MIP_COUNT; i++)
            stateArr[i] = value;
        
        return stateArr;
    }
}

#endif