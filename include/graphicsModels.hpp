#ifndef GRAPHICS_MODELS_HPP
#define GRAPHICS_MODELS_HPP
 
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#include <vulkan/vulkan_raii.hpp>

// Forces GLM to use a type versions with alignment requirements that are
// compatible with Vulkan specification: https://docs.vulkan.org/spec/latest/chapters/interfaces.html#interfaces-resources-layout
// #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <array>

using std::array;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return {
            .binding   = 0, // can be used to describe separate buffers per attribute (vertex, color) or a separate buffer to stream per instance transformation data
            .stride    = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
        return {{
            {.location = 0, 
             .binding  = 0, 
             .format   = vk::Format::eR32G32B32Sfloat,
             .offset   = offsetof(Vertex, pos)},
            
            {.location = 1,
             .binding  = 0,
             .format   = vk::Format::eR32G32B32Sfloat, 
             .offset   = offsetof(Vertex, color)},
            
            {.location = 2,
             .binding  = 0,
             .format   = vk::Format::eR32G32B32Sfloat, 
             .offset   = offsetof(Vertex, texCoord)}
        }};
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

// UniformBuffer struct with explicit alignment specifier (C++11)
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

enum CommandIntentHint {
    GRAPHICS = 0,
    TRANSFER = 1
};

const std::vector<Vertex> quadVertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> quadIndices  = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

#endif