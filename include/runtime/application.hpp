#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#define GLFW_INCLUDE_VULKAN //? TODO: Is this defined somewhere else?
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <runtime/applicationModels.hpp>
#include <version.hpp>
#include <config.hpp>

using std::cerr;
using std::endl;
using std::vector;

using Runtime::Models::WindowState;
using Runtime::Models::WindowParameters;

namespace Runtime {
    class Application {
        public:
            static Application& getInstance();

            Application(const Application&) = delete;
            Application(Application&&) noexcept = delete;

            ~Application();

            void update();

            Application& operator=(const Application&) = delete;
            Application& operator=(Application&&) noexcept = delete;

            GLFWwindow* getWindow() const;

            void getFramebufferSize(int *width, int *height);

            void addUpdateListener(void (*listener)());
            void removeUpdateListener(void (*listener)());

            const bool shouldClose() const;
            
            vector<const char*> getRequiredExtensions(uint32_t* count) const;
            const char* name() const;

            const bool isWindowDirty() const;
            const WindowState getWindowState() const;

            const double getTime() const;
            const double getDeltaTime() const;
        private:
            Application();        

            GLFWwindow* window;

            vector<void(*)()> updateListeners;

            bool windowDirty = true;
            WindowState windowState = WindowState::WINDOW_NULL;

            static void error_callback(const int error, const char* description);
            static void key_callback(GLFWwindow* window, const int key, const int scancode, const int action, const int mods);
            static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);
            void update_callback() const;

            double time;
            double deltaTime;
    };
}

#endif