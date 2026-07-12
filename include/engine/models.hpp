#ifndef ENGINE_MODELS_HPP
#define ENGINE_MODELS_HPP

#include <vk_mem_alloc.h>

namespace Engine::Models {
    struct Mesh {
        VmaAllocation vertexAllocation;
        VmaAllocation indexAllocation;

        VkBuffer vertexBuffer;
        VkBuffer indexBuffer;

        uint32_t vertexCount;
        uint32_t indexCount;
    };

    struct Texture {
        VmaAllocation allocation;

        VkImage image;
        VkImageView view;
        VkSampler sampler;

        uint32_t width;
        uint32_t height;
        uint32_t channels;
    };

    struct Shader {
        VkShaderModule shaderModule;
        VkShaderStageFlagBits stage;
    };
}

#endif