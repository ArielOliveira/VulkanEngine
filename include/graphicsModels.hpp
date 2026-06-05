#ifndef GRAPHICS_MODELS_HPP
#define GRAPHICS_MODELS_HPP
 
#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>

#include <array>

using std::array;

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return {
            .binding   = 0, // can be used to describe separate buffers per attribute (vertex, color) or a separate buffer to stream per instance transformation data
            .stride    = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
        return {{
            {.location = 0, 
             .binding  = 0, 
             .format   = vk::Format::eR32G32Sfloat,
             .offset   = offsetof(Vertex, pos)},
            
            {.location = 1,
             .binding  = 0,
             .format   = vk::Format::eR32G32B32Sfloat, 
             .offset   = offsetof(Vertex, color)}
        }};
    }
};

const std::vector<Vertex> vertices = {
    {{ 0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},
};

#endif