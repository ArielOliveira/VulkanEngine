#include <engine/fileParser.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <runtime/fileHelper.hpp>

#include <filesystem>
#include <vector>
#include <stack>


namespace Engine {
    const std::pair<Parser, Expected<Asset>>  FileParser::openFile(const std::string& path) {
        Parser parser;

        std::filesystem::path systemPath = path;
        
        auto data = fastgltf::GltfDataBuffer::FromPath(systemPath);

        if (data.error() != fastgltf::Error::None)
            throw std::runtime_error("Couldn't load file from path: " + systemPath.string() + " Error Code: " + std::to_string((int)data.error()));

        auto asset = parser.loadGltf(data.get(), systemPath.parent_path(), fastgltf::Options::None);

        if (auto error = asset.error(); error != fastgltf::Error::None) 
            throw std::runtime_error("Couldn't parse file from path: " + systemPath.string() + " Error Code: " + std::to_string((int)error));
        
        return { std::move(parser), std::move(asset) };
    }

    const ModelImportResult FileParser::parseModel(const Parser& parser, const Asset& asset) {        
        ModelImportResult importResult = {
            .sceneDescriptors = std::vector<SceneDescriptor>(asset.scenes.size())
        };

        std::cout << "Found " << asset.scenes.size() << " scene(s)" << '\n';

        for (size_t sceneIndex = 0; sceneIndex < asset.scenes.size(); sceneIndex++) {
            auto scene = asset.scenes[sceneIndex];
            
            SceneDescriptor& sceneDesc = importResult.sceneDescriptors.emplace_back(
                scene.name.c_str(),
                sceneIndex,
                std::vector<std::string>(scene.nodeIndices.size()),
                std::vector<uint32_t>(scene.nodeIndices.size()),
                std::vector<glm::mat4x4>(scene.nodeIndices.size()),
                std::vector<std::pair<uint32_t, size_t>>()
            );

            std::stack<std::tuple<size_t, std::optional<size_t>, uint32_t>> nodeStack;
            
            for (auto nodeIndex : scene.nodeIndices | std::views::reverse)
                nodeStack.push({ nodeIndex, {}, 0 });

            while(!nodeStack.empty()) {
                auto [current, parent, hierarchyLayer]  = nodeStack.top();
                auto& node                              = asset.nodes[current];
                nodeStack.pop();
                
                sceneDesc.nodeNames.emplace_back(node.name.c_str());
                sceneDesc.hierarchy.push_back(hierarchyLayer-1);
                sceneDesc.transforms.emplace_back(glm::make_mat4x4(fastgltf::getTransformMatrix(node).data()));
                
                if (node.meshIndex.has_value())
                    sceneDesc.meshDependency.emplace_back(sceneDesc.hierarchy.size()-1, node.meshIndex.value());
                
                for (auto childIndex : node.children | std::views::reverse)
                    nodeStack.push({ childIndex, current, hierarchyLayer+1 });
            }
        }

        return importResult;
    }

    const Mesh FileParser::parseMesh(const Asset& asset, const size_t meshIndex) {
        for (auto& mesh : asset.meshes) {
            for (auto& primitive : mesh.primitives) {
                auto* positionIt = primitive.findAttribute("POSITION");
                auto& positionAccessor = asset.accessors[positionIt->accessorIndex];

                if (!positionAccessor.bufferViewIndex.has_value()) continue;
                   
                fastgltf::iterateAccessor<glm::vec3>(asset, positionAccessor, [&](glm::vec3 position) {
                    
                });
            }
        }
    }
}