#include <iostream>
#include <version.hpp>
#include <application.hpp>
#include <graphics.hpp>
#include <fileHelper.hpp>

using std::cout;
using std::endl;

int main(int argc, char** args) {    
    Application& application = Application::getInstance();
    Graphics& graphics = Graphics::getInstance();

    while(!application.shouldClose()) {
        application.update();
    }

    return 0;
}