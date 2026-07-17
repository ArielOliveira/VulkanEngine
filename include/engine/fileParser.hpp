#ifndef ENGINE_FILE_PARSER_HPP
#define ENGINE_FILE_PARSER_HPP

#include <string>

#include <fastgltf/core.hpp>
using fastgltf::Parser;
using fastgltf::Expected;
using fastgltf::Asset;

#include <engine/resources.hpp>
using namespace Engine::Resources::Descriptors;
using namespace Engine::Resources;

namespace Engine::Paths {
    constexpr const char* MODELS            = "models/";
    constexpr const char* TEXTURES          = "textures/";
    constexpr const char* SHADERS           = "./spirv/";
}

namespace Engine::FileParser {
    const std::pair<Parser, Expected<Asset>>  openFile(const std::string& path);
    const ModelImportResult                   parseModel(const Parser& parser, const Asset& asset);
    const Mesh                                parseMesh(const Asset& asset, const size_t meshIndex);
}

#endif