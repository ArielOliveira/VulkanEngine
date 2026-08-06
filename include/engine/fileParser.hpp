#ifndef ENGINE_FILE_PARSER_HPP
#define ENGINE_FILE_PARSER_HPP

#include <filesystem>

#include <string>

#include <fastgltf/core.hpp>
#include <engine/descriptors.hpp>

using fastgltf::Parser;
using fastgltf::Expected;
using fastgltf::Asset;

using Engine::Descriptors::TextureAsset;

namespace Engine::Paths {
    const std::filesystem::path MODELS            = "../../models/";
    const std::filesystem::path TEXTURES          = "../../textures/";
    const std::filesystem::path SHADERS           = "./spirv/";
}

namespace Engine::FileParser {
    const std::tuple<Parser, Expected<Asset>, std::string>  openModelFile(const std::string& path);
    const std::pair<TextureAsset, void*> openTextureFile(const std::string& path);
    void closeTextureFile(void* handle);
}

#endif