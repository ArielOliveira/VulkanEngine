#include <iostream>

#include <version.hpp>

#include <runtime/application.hpp>

#include <graphics/core.hpp>
#include <graphics/renderer.hpp>

#include <engine/resourceManager.hpp>

using Graphics::Core;
using Graphics::Renderer;

using Runtime::Application;

using Engine::ResourceManager;

using std::cout;
using std::endl;

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