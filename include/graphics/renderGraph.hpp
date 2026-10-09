#ifndef GRAPHICS_RENDER_GRAPH_HPP
#define GRAPHICS_RENDER_GRAPH_HPP

#include <graphics/commandBuffer.hpp>
#include <engine/resources.hpp>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <typeindex>

using namespace Engine::Resources::GPU;
using Engine::ResourceHandle;

namespace Graphics {
    class RenderGraph;

    using pass_index           = uint16_t;
    using resource_index       = uint32_t;

    typedef std::vector<resource_index> attachment_list;
    typedef std::vector<resource_index> buffer_list;
    
    using resource_index_pools = std::tuple<attachment_list, buffer_list>;

    enum ResourceType {
        Attachment = 0,
        Buffer     = 1
    };

    struct AttachmentInfo {
        uint32_t width, height, depth;
        uint32_t samples;
        uint32_t mips;
        uint32_t layers;

        VkFormat          format;
        VkImageUsageFlags usage;

        bool operator==(const AttachmentInfo& other) const {
            return width   == other.width   &&
                   height  == other.height  &&
                   depth   == other.depth   &&
                   samples == other.samples &&
                   mips    == other.mips    &&
                   layers  == other.layers  &&
                   format  == other.format  &&
                   usage   == other.usage;
            }

        bool operator!=(const AttachmentInfo& other) const {
            return !(*this == other);
        }
    };

    struct BufferInfo {
        VkDeviceSize size;
        VkBufferUsageFlags2 usage;

        bool operator==(const BufferInfo& other) const {
            return size  == other.size &&
                   usage == other.usage;
        }

        bool operator!=(const BufferInfo& other) const {
            return !(*this == other);
        }
    };

    template <typename ResourceType>
    struct RenderResource {
        std::string name;

        ResourceType data;
    };

    class RenderPass {
        public:
            RenderPass() = delete;
            RenderPass(const std::string& _name, 
                       const RenderGraph& _graph, 
                       const pass_index _index,
                       std::function<void (Graphics::CommandBuffer&)> _build_function);
            
            void addColorInput(const std::string& name);
            void addColorOutput(const std::string& name);

            void addBufferInput(const std::string& name);
            void addBufferOutput(const std::string& name);

            const std::string& getName()  const { return name; }
            const RenderGraph& getGraph() const { return graph; }
            const pass_index   getIndex() const { return index; }

            template <typename ResourceType>
            const std::vector<resource_index>& getInputs() const;

            template <typename ResourceType>
            const std::vector<resource_index>& getOutputs() const; 
        private:
            const std::string name;
            const RenderGraph& graph;
            const pass_index index;

            std::tuple<attachment_list, buffer_list> inputs;
            std::tuple<attachment_list, buffer_list> outputs;

            std::function<void (Graphics::CommandBuffer&)> build_function;
    };

    class RenderGraph {
        public:
            using NameToIndexMap = std::unordered_map<std::string, resource_index>;

            RenderGraph();
            
            RenderPass& addPass(const std::string& name, std::function<void (Graphics::CommandBuffer&)> build_function);

            template <typename ResourceType>
            const std::pair<ResourceType&, const resource_index> getResource(const std::string& name);

            template <typename ResourceType>
            const ResourceType& getResource(const resource_index index) const; 

            template <typename ResourceType>
            ResourceType& getResource(const resource_index index);

            void compile();
            void build();
            void execute() const;
        private:
            using AttachmentMap = NameToIndexMap;
            using BufferMap     = NameToIndexMap;

            const std::string backbuffer = "root";

            std::unordered_map<std::string, pass_index>                      passToIndex;
            std::tuple<AttachmentMap, BufferMap>                             resourceToIndex;

            std::vector<RenderPass>                                          declaredPasses;
            std::vector<pass_index>                                          executionOrder;
            std::vector<uint8_t>                                             passDependencyLevel;

            std::tuple<std::vector<AttachmentInfo>, std::vector<BufferInfo>> declaredResources;

            template <typename ResourceType>
            std::vector<ResourceType>& getResources();

            template <typename ResourceType>
            NameToIndexMap& getMap(); 

            void sortDependencies();
    };
}

#include <graphics/renderGraph.tpp>

#endif