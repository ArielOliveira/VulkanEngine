#ifndef GRAPHICS_PIPELINE_HPP
#define GRAPHICS_PIPELINE_HPP

#include <graphics/models.hpp>
#include <fileHelper.hpp>

using std::move;
using std::swap;

namespace Graphics {
    class Pipeline {
        public:
            Pipeline() = delete;
            ~Pipeline();

            Pipeline(const vk::raii::Device &device, const vk::Extent2D &swapChain, const vk::SurfaceFormatKHR &swapChainSurfaceFormat, vk::Format depthFormat);

            Pipeline(const Pipeline&) = delete;
            Pipeline(Pipeline &&other) noexcept = default;
            Pipeline(std::nullptr_t) noexcept;

            Pipeline& operator=(const Pipeline&) = delete;
            Pipeline& operator=(Pipeline &&other) noexcept = default;
            Pipeline& operator=(std::nullptr_t) noexcept;

            const char* shaderRelativePath = "./spirv/";

            const vk::raii::Pipeline&  getInstance() const &;
            const vk::raii::PipelineLayout&      getPipelineLayout() const &;
            const vk::raii::DescriptorSetLayout& getDescriptorSetLayout() const &;
            
            void createDescriptorPool(const vk::raii::Device &device);
            void createDescriptorSets(const vk::raii::Device &device, const vector<vk::raii::Buffer> &uniformBuffers, const vk::DescriptorImageInfo &imageInfo);

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