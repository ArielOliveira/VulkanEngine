#include <graphics/renderGraph.hpp>

namespace Graphics {
    RenderGraph::RenderGraph() {}

    RenderPass& RenderGraph::addPass(const std::string& name, std::function<void (Graphics::CommandBuffer&)> build_function) {
        auto it = passToIndex.find(name);
        if (it != passToIndex.end())
            return declaredPasses[it->second];

        declaredPasses.emplace_back(RenderPass(name, *this, declaredPasses.size(), build_function));
        passToIndex[name] = declaredPasses.size();

        return declaredPasses[declaredPasses.size()-1];
    }

    std::tuple<std::vector<std::unordered_set<pass_index>>, 
               std::vector<std::unordered_set<pass_index>>> RenderGraph::sortDependencies() {
        std::vector<std::unordered_set<pass_index>> dependencies(declaredPasses.size()); // What each pass depends on
        std::vector<std::unordered_set<pass_index>> dependents(declaredPasses.size());   // What depends on each pass

        std::unordered_map<std::type_index, std::unordered_map<resource_index, pass_index>> resourceWriters;

        // step 1 - dependency graph building
        for (pass_index i = 0; i < declaredPasses.size(); i++) {
            for (const auto& output : declaredPasses[i].getOutputs<AttachmentInfo>()) 
                resourceWriters[typeid(AttachmentInfo)][output] = i;

            for (const auto& output : declaredPasses[i].getOutputs<BufferInfo>()) 
                resourceWriters[typeid(BufferInfo)][output] = i;
        }

        for (pass_index i = 0; i < declaredPasses.size(); i++) {
            const RenderPass& pass = declaredPasses[i];

            if (resourceWriters.contains(typeid(AttachmentInfo))) {
                auto& attachmentWriters = resourceWriters[typeid(AttachmentInfo)];

                for (const auto& input : pass.getInputs<AttachmentInfo>()) {
                    auto it = attachmentWriters.find(input);
                    if (it != attachmentWriters.end()) {
                        dependencies[i].insert(it->second);
                        dependents[it->second].insert(i);
                    }
                }
            }

            if (resourceWriters.contains(typeid(BufferInfo))) {
                auto& bufferWriters = resourceWriters[typeid(BufferInfo)];

                for (const auto& input : pass.getInputs<BufferInfo>()) {
                    auto it = bufferWriters.find(input);
                    if (it != bufferWriters.end()) {
                       dependencies[i].insert(it->second);
                       dependents[it->second].insert(i);
                    }
                }
            }
        }

        // step 2 - DFS topological order
        std::vector<bool> visited(declaredPasses.size(), false);
        std::vector<bool> inStack(declaredPasses.size(), false);

        std::function<void(pass_index)> visit = [&](pass_index node) {
            if (inStack[node])
                throw std::runtime_error("Cycle detected in rendergraph");

            if (visited[node])
                return;

            inStack[node] = true;

            for (auto dependency : dependencies[node])
                visit(dependency);

            inStack[node] = false;
            visited[node] = true;
            executionOrder.push_back(node);
        };

        for (pass_index i = 0; i < declaredPasses.size(); i++) {
            if (!visited[i])
                visit(i);
        }

        // step 3 - longest path using the resulting DFS topological order
        passDependencyLevel.clear();
        passDependencyLevel.resize(declaredPasses.size(), 0);
        for (auto pass : executionOrder) {
            for (auto dependency : dependencies[pass]) {
                passDependencyLevel[dependency] = 
                    std::max(passDependencyLevel[dependency], (uint8_t)(passDependencyLevel[pass] + 1U));
            }
        }

        return { std::move(dependencies), std::move(dependents) };
    }

    void RenderGraph::compile() {
        auto [dependencies, dependents] = sortDependencies();
        
        
    }

    void RenderGraph::build() {}

    void RenderGraph::execute() const {}
}

namespace Graphics {
    RenderPass::RenderPass(const std::string& _name, 
                           const RenderGraph& _graph,
                           const pass_index   _index,
                           const std::function<void (Graphics::CommandBuffer&)> _build_function) 
                        : name(_name)
                        , graph(_graph)
                        , index(_index)
                        , build_function(_build_function) {}
}