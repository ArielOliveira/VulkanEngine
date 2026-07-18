#ifndef ENGINE_FILE_PARSER_HPP
#define ENGINE_FILE_PARSER_HPP

#include <string>

#include <fastgltf/core.hpp>
using fastgltf::Parser;
using fastgltf::Expected;
using fastgltf::Asset;

namespace Engine::Paths {
    constexpr const char* MODELS            = "models/";
    constexpr const char* TEXTURES          = "textures/";
    constexpr const char* SHADERS           = "./spirv/";
}

namespace Engine::FileParser {
    const std::tuple<Parser, Expected<Asset>, std::string>  openFile(const std::string& path);
}

#endif