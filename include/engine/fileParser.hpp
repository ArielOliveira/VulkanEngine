#ifndef ENGINE_FILE_PARSER_HPP
#define ENGINE_FILE_PARSER_HPP

#include <string>

#include <fastgltf/core.hpp>
using fastgltf::Parser;
using fastgltf::Expected;
using fastgltf::Asset;

#include <filesystem>

namespace Engine::Paths {
    const std::filesystem::path MODELS            = "../../models/";
    const std::filesystem::path TEXTURES          = "../../textures/";
    const std::filesystem::path SHADERS           = "./spirv/";
}

namespace Engine::FileParser {
    const std::tuple<Parser, Expected<Asset>, std::string>  openFile(const std::string& path);
}

#endif