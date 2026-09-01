#include <iostream>

#include <version.hpp>

#include <runtime/application.hpp>

#include <graphics/core.hpp>
#include <graphics/renderer.hpp>

#include <engine/resourceManager.hpp>
#include <engine/fileParser.hpp>

#include <utils/sparseset.hpp>

using Graphics::Core;
using Graphics::Renderer;

using Runtime::Application;

using Engine::ResourceManager;

using Utils::SparseSet;
using Utils::Types::SlotKey;

using std::cout;
using std::endl;
using std::string;

class Test {
    public:
        const SlotKey registerEntity(const uint32_t characterCount) {
            const SlotKey key = m_slotMap.emplace(characterCount);

            string s;
            
            uint32_t firstAlphabet = static_cast<uint32_t>('A');
            uint32_t alphabetCount = static_cast<uint32_t>('Z' - 'A');

            for (uint32_t i = 0; i < characterCount; i++) {
                char roundTrip = static_cast<char>(i / alphabetCount);
                char alphabet  = static_cast<char>((i % alphabetCount) + firstAlphabet);

                s += alphabet;
            }

            m_sparseSet.emplace(key, s);

            return key;
        }

        void unregisterEntity(const SlotKey& key) {
            assert(m_slotMap[key] == m_sparseSet[key].size());

            m_sparseSet.erase(key);
            m_slotMap.erase(key);
        }
        
    private: 
        SlotMap<uint32_t> m_slotMap;
        SparseSet<string> m_sparseSet;

};

int main(int argc, char** args) { 
    ResourceManager& rm = ResourceManager::getInstance();

    Application& application = Application::getInstance();
    Core& core = Core::getInstance();
    Renderer& renderer = Renderer::getInstance();

    while(!application.shouldClose()) {
        application.update();
        renderer.drawFrame();
    }

    core.getDevice().waitIdle();

    return 0;
}