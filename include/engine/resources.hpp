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
        uint32_t mipCount;
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
}

#endif