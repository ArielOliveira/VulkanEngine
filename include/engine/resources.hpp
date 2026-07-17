#ifndef ENGINE_RESOURCES_HPP
#define ENGINE_RESOURCES_HPP

#include <vk_mem_alloc.h>

#include <vector>
using std::vector;

#include <glm/glm.hpp>


#include <utils/slotmap.hpp>
using Utils::SlotKey;

namespace Engine::Resources::Descriptors {
    struct SceneDescriptor {
        std::string name;

        size_t                      resourceIndex;            
        
        vector<std::string>         nodeNames;
        vector<uint32_t>            hierarchy;
        vector<glm::mat4x4>         transforms;
        
        vector<std::pair<uint32_t, size_t>> meshDependency;
    };

    struct ModelImportResult {
        vector<SceneDescriptor> sceneDescriptors;
    };

    struct ModelImportParameters {

    };
}

namespace Engine::Resources {
    struct Model {
        vector<SlotKey> scenes;
    };

    struct Scene {
        vector<uint32_t> hierarchy;
        vector<glm::mat4x4> transforms;
        vector<std::optional<SlotKey>> meshHandle;
        vector<std::optional<SlotKey>> animationHandle;
    };

    struct Mesh {
        VmaAllocation vertexAllocation;
        VmaAllocation indexAllocation;

        VkBuffer vertexBuffer;
        VkBuffer indexBuffer;

        uint32_t vertexCount;
        uint32_t indexCount;

        // Submesh data
        vector<uint32_t> smIndexOffset;
        vector<uint32_t> smIndexCount;
        vector<uint32_t> smVertexOffset;
        vector<SlotKey>  smMaterialHandle;
    };

    struct Animation {

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

    struct Material {

    };

    struct Shader {
        VkShaderModule shaderModule;
        VkShaderStageFlagBits stage;
    };
}

#endif