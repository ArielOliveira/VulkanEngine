#ifndef GRAPHICS_PIPELINE_HPP
#define GRAPHICS_PIPELINE_HPP

#include <graphics/models.hpp>
#include <application/fileHelper.hpp>

using std::move;
using std::swap;

namespace Graphics {
    class Pipeline {
        public:
            Pipeline() = delete;
            ~Pipeline();

            Pipeline(const vk::raii::Device &device, const vk::ComputePipelineCreateInfo &createInfo, vk::raii::PipelineLayout& pipelineLayout, vk::raii::DescriptorSetLayout &descriptorSetLayout);
            Pipeline(const vk::raii::Device &device, const vk::GraphicsPipelineCreateInfo &createInfo, vk::raii::PipelineLayout& pipelineLayout, vk::raii::DescriptorSetLayout &descriptorSetLayout);

            Pipeline(const Pipeline&) = delete;
            Pipeline(Pipeline &&other) noexcept = default;
            Pipeline(std::nullptr_t) noexcept;

            static Pipeline createGraphicsPipeline(const vk::raii::Device &device, const vk::Extent2D &swapChain, const vk::SurfaceFormatKHR &swapChainSurfaceFormat, vk::Format depthFormat, vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1);
            static Pipeline createComputePipeline(const vk::raii::Device &device);

            Pipeline& operator=(const Pipeline&) = delete;
            Pipeline& operator=(Pipeline &&other) noexcept = default;
            Pipeline& operator=(std::nullptr_t) noexcept;

            const vk::raii::Pipeline&            getInstance() const &;
            const vk::raii::PipelineLayout&      getPipelineLayout() const &;
            const vk::raii::DescriptorSetLayout& getDescriptorSetLayout() const &;
            
            void createGraphicsDescriptorPool(const vk::raii::Device &device);
            void createGraphicsDescriptorSets(const vk::raii::Device &device, const vector<vk::raii::Buffer> &uniformBuffers, const vk::DescriptorImageInfo &imageInfo);

            void createComputeDescriptorPool(const vk::raii::Device &device);
            void createComputeDescriptorSets(const vk::raii::Device &device, const vector<vk::raii::Buffer> &shaderStorageBuffers);

            const vk::raii::DescriptorSet& getDescriptorSet(uint32_t frameIndex) const;
        private:
            vk::raii::Pipeline pipeline                       = nullptr;
            vk::raii::PipelineLayout pipelineLayout           = nullptr;
            vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
            vk::raii::DescriptorPool descriptorPool           = nullptr;
            
            vector<vk::raii::DescriptorSet> descriptorSets;
    };
}
#endif