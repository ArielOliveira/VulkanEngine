#ifndef APPLICATION_MODELS
#define APPLICATION_MODELS

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