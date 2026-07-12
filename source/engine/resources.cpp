#include <engine/resources.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <runtime/fileHelper.hpp>

#include <filesystem>


namespace Resources {
    void MeshLoader::loadMesh(const std::string& name) {
        fastgltf::Parser parser;

        std::filesystem::path path = std::string(Paths::MODELS) + name;
        
        auto data = fastgltf::GltfDataBuffer::FromPath(path);

        if (data.error() != fastgltf::Error::None)
            throw std::runtime_error("Couldn't load file from path: " + path.string() + " Error Code: " + std::to_string((int)data.error()));

        auto asset = parser.loadGltf(data.get(), path.parent_path(), fastgltf::Options::None);

        if (auto error = asset.error(); error != fastgltf::Error::None) 
            throw std::runtime_error("Couldn't parse file from path: " + path.string() + " Error Code: " + std::to_string((int)error));

        std::cout << "Found " << asset.get().meshes.size() << " meshe(s) in model " << name << '\n';

        for (auto& mesh : asset.get().meshes) {
            for (auto& primitive : mesh.primitives) {
                auto* positionIt = primitive.findAttribute("POSITION");
                auto& positionAccessor = asset.get().accessors[positionIt->accessorIndex];

                if (!positionAccessor.bufferViewIndex.has_value()) continue;
                   
                
            }
        }
    }
}