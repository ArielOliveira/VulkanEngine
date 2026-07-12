#ifndef RESOURCES_HPP
#define RESOURCES_HPP

#include <string>

namespace Resources::Paths {
    constexpr const char* MODELS            = "models/";
    constexpr const char* TEXTURES          = "textures/";
    constexpr const char* SHADERS           = "./spirv/";
}

namespace Resources::MeshLoader {
    void loadMesh(const std::string& name);
}

namespace Resources::TextureLoader {

}

namespace Resources::ShaderLoader {
    
}

#endif