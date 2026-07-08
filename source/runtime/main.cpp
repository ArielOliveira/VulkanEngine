#include <iostream>
#include <graphics/core.hpp>
#include <graphics/renderer.hpp>
#include <engine/resourceManager.hpp>
#include <version.hpp>
#include <runtime/application.hpp>
#include <runtime/fileHelper.hpp>
#include <utils/slotmap.hpp>

using Graphics::Core;
using Graphics::Renderer;

using Runtime::Application;

using Engine::ResourceManager;

using Utils::SlotMap;
using Utils::Key;

using std::cout;
using std::endl;

int main(int argc, char** args) { 
    SlotMap<std::string> slotMap;

    slotMap.insert("A");
    slotMap.insert("B");
    slotMap.insert("C");
    slotMap.insert("D");
    Key eKey = slotMap.insert("E");
    slotMap.insert("F");
    slotMap.insert("G");
    slotMap.insert("H");

    slotMap.erase(eKey);

    /*Application& application = Application::getInstance();
    ResourceManager& rm = ResourceManager::getInstance();
    Core& core = Core::getInstance();
    Renderer& renderer = Renderer::getInstance();

    while(!application.shouldClose()) {
        application.update();
        renderer.drawFrame();
    }

    core.getDevice().waitIdle();*/

    return 0;
}