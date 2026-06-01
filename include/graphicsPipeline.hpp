#ifndef GRAPHICS_PIPELINE_HPP
#define GRAPHICS_PIPELINE_HPP

#include <fileHelper.hpp>

#include <vulkan\vulkan_raii.hpp>

using std::move;
using std::swap;

using vk::raii::Device;
using vk::raii::ShaderModule;
using vk::raii::PipelineLayout;
using vk::SurfaceFormatKHR;
using vk::Extent2D;

class GraphicsPipeline {
    public:
        GraphicsPipeline() = delete;
        ~GraphicsPipeline();

        GraphicsPipeline(const Device &device, const Extent2D &swapChain, const SurfaceFormatKHR &swapChainSurfaceFormat);
        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline(GraphicsPipeline &&other) noexcept;
        GraphicsPipeline(std::nullptr_t) noexcept;

        GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator=(GraphicsPipeline &&other) noexcept;
        GraphicsPipeline& operator=(std::nullptr_t) noexcept;

        const char* shaderRelativePath = "./spirv/";
    
    private:
        vk::raii::Pipeline pipeline   = nullptr;
        PipelineLayout pipelineLayout = nullptr;

};

#endif