#ifndef GRAPHICS_PIPELINE_HPP
#define GRAPHICS_PIPELINE_HPP

#include <graphicsModels.hpp>
#include <fileHelper.hpp>

#include <vulkan\vulkan_raii.hpp>

using std::move;
using std::swap;


// RAII class members are redundant, the class object
// should now be responsible for its members lifetime.
using vk::raii::Device;
using vk::raii::ShaderModule;
using vk::raii::PipelineLayout;
using vk::raii::DescriptorSetLayout;
using vk::raii::DescriptorPool;
using vk::raii::DescriptorSet;
using vk::SurfaceFormatKHR;
using vk::Extent2D;

class GraphicsPipeline {
    public:
        GraphicsPipeline() = delete;
        ~GraphicsPipeline();

        GraphicsPipeline(const Device &device, const Extent2D &swapChain, const SurfaceFormatKHR &swapChainSurfaceFormat, vk::Format depthFormat);

        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline(GraphicsPipeline &&other) noexcept = default;
        GraphicsPipeline(std::nullptr_t) noexcept;

        GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator=(GraphicsPipeline &&other) noexcept = default;
        GraphicsPipeline& operator=(std::nullptr_t) noexcept;

        const char* shaderRelativePath = "./spirv/";

        const vk::raii::Pipeline&  getInstance() const &;
        const PipelineLayout&      getPipelineLayout() const &;
        const DescriptorSetLayout& getDescriptorSetLayout() const &;
        
        void createDescriptorPool(const Device &device);
        void createDescriptorSets(const Device &device, const vector<vk::raii::Buffer> &uniformBuffers, const vk::DescriptorImageInfo &imageInfo);

        const DescriptorSet& getDescriptorSet(uint32_t frameIndex) const;
    private:
        vk::raii::Pipeline pipeline             = nullptr;
        PipelineLayout pipelineLayout           = nullptr;
        DescriptorSetLayout descriptorSetLayout = nullptr;
        DescriptorPool descriptorPool           = nullptr;
        
        vector<DescriptorSet> descriptorSets;
};

#endif