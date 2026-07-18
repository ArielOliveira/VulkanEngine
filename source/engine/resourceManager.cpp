#include <engine/resourceEngine.hpp>

using Engine::Resources::Model;

namespace Engine {
    ResourceManager& ResourceManager::getInstance() {
        static ResourceManager instance;

        return instance;
    }

    ResourceManager::ResourceManager() { 
        // Ensures order of initialization 
        // GPU memory allocator is guaranteed to be initialized by the time 
        // it is needed
        GPUMemoryManager::getInstance(); 
    }

    ResourceManager::~ResourceManager() {}

    const ResourceHandle<Model> ResourceManager::load(const std::string& path) {
        assert(findKeyByName(std::type_index(typeid(Model)), path) == nullptr);

        std::cout << "Loading model: " << path << '\n';
    
        auto [parser, asset, fileName] = FileParser::openFile(path);

        Model model = {
            .name       = fileName.c_str(),
            .scenes     = std::vector<ResourceHandle<Scene>>()
        };

        model.scenes.reserve(asset.get().scenes.size());
        
        for (size_t sceneIndex = 0; sceneIndex < asset.get().scenes.size(); sceneIndex++) {
            model.scenes.emplace_back(load<Scene>(fileName, asset.get(), sceneIndex));
        }

        std::cout << "Registering model " << path << '\n';
        SlotKey key = pools.get<Model>().insert(model);
        
        cache[std::type_index(typeid(Model))][model.name] = key;

        return std::move(ResourceHandle<Model>(key));
    }

    SlotKey* const ResourceManager::findKeyByName(const std::type_index& typeId, const std::string& name) { 
        auto it = cache.find(typeId);
        
        if (it == cache.end()) return nullptr;

        try {
            return &it->second.at(name);
        } catch(std::exception e) {
            return  nullptr;
        }
    }
}