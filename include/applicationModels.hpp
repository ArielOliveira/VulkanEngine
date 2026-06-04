#ifndef APPLICATION_MODELS
#define APPLICATION_MODELS

enum WindowState {
    WINDOW_NULL = 0,
    WINDOW_ON_FOCUS = 1
};

enum Monitor {
    PRIMARY = 0,
    SECONDARY = 1
};

struct WindowParameters {
    unsigned width = 800, height = 600;
    const char* title = "VulkanEngine";
    unsigned monitor = 0;
    void* sharedContext = nullptr;
};

#endif