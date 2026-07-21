#include <engine/resourceEngine.hpp>

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