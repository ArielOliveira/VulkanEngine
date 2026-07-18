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
    const std::tuple<Parser, Expected<Asset>, std::string>  FileParser::openFile(const std::string& path) {
        Parser parser;

        std::filesystem::path systemPath = path;
        
        auto data = fastgltf::GltfDataBuffer::FromPath(systemPath);

        if (data.error() != fastgltf::Error::None)
            throw std::runtime_error("Couldn't load file from path: " + systemPath.string() + " Error Code: " + std::to_string((int)data.error()));

        auto asset = parser.loadGltf(data.get(), systemPath.parent_path(), fastgltf::Options::None);

        if (auto error = asset.error(); error != fastgltf::Error::None) 
            throw std::runtime_error("Couldn't parse file from path: " + systemPath.string() + " Error Code: " + std::to_string((int)error));
        
        return { std::move(parser), std::move(asset), systemPath.filename().string() };
    }
}