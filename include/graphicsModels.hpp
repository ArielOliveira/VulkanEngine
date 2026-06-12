#ifndef GRAPHICS_MODELS_HPP
#define GRAPHICS_MODELS_HPP
 

#include <vulkan/vulkan_raii.hpp>
// Forces GLM to use a type versions with alignment requirements that are
// compatible with Vulkan specification: https://docs.vulkan.org/spec/latest/chapters/interfaces.html#interfaces-resources-layout
// #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <array>

using std::array;

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
};

// UniformBuffer struct with explicit alignment specifier (C++11)
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

const std::vector<Vertex> planeVertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> planeIndices  = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

#endif