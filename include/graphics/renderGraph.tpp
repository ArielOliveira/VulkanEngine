#ifndef GRAPHICS_RENDER_GRAPH_TPP
#define GRAPHICS_RENDER_GRAPH_TPP

#include <graphics/renderGraph.hpp>

namespace Graphics {
    template <typename ResourceType>
    inline const std::pair<ResourceType&, const resource_index> RenderGraph::getResource(const std::string& name) {
        auto it = getMap<ResourceType>().find(name);

        if (it != getMap<ResourceType>().end()) 
            return it->second;

        resource_index index = getResources<ResourceType>().size();
        
        getMap<ResourceType>()[name] = index;
        getResources<ResourceType>().emplace_back( ResourceType {} );

        return { getResources<ResourceType>()[index], index };
    } 

    template <typename ResourceType>
    inline const ResourceType& RenderGraph::getResource(const resource_index index) const { 
        assert((index >= 0 && index < getResources<ResourceType>().size()) && "INDEX OUT OF BOUNDS");

        return getResources<ResourceType>()[index]; 
    }

    template <typename ResourceType>
    inline ResourceType& RenderGraph::getResource(const resource_index index) {
        assert((index >= 0 && index < getResources<ResourceType>().size()) && "INDEX OUT OF BOUNDS");

        return getResources<ResourceType>()[index];
    }

    template <>
    inline std::vector<AttachmentInfo>& RenderGraph::getResources<AttachmentInfo>() {
        return std::get<std::vector<AttachmentInfo>>(declaredResources);
    }

    template <>
    inline std::vector<BufferInfo>& RenderGraph::getResources<BufferInfo>() {
        return std::get<std::vector<BufferInfo>>(declaredResources);
    }

    template <>
    inline RenderGraph::NameToIndexMap& RenderGraph::getMap<AttachmentInfo>() {
        return std::get<ResourceType::Attachment>(resourceToIndex);
    }

    template <>
    inline RenderGraph::NameToIndexMap& RenderGraph::getMap<BufferInfo>() {
        return std::get<ResourceType::Buffer>(resourceToIndex);
    }
}

namespace Graphics {
    template <>
    inline const std::vector<resource_index>& RenderPass::getInputs<AttachmentInfo>() const {
        return std::get<ResourceType::Attachment>(inputs);
    }

    template <>
    inline const std::vector<resource_index>& RenderPass::getInputs<BufferInfo>() const {
        return std::get<ResourceType::Buffer>(inputs);
    }

    template <>
    inline const std::vector<resource_index>& RenderPass::getOutputs<AttachmentInfo>() const {
        return std::get<ResourceType::Attachment>(outputs);
    }

    template <>
    inline const std::vector<resource_index>& RenderPass::getOutputs<BufferInfo>() const {
        return std::get<ResourceType::Buffer>(outputs);
    }


}

#endif