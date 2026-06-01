find_package(Vulkan REQUIRED)

add_library(${LIB_VULKAN})
add_library(Vulkan::cppm ALIAS ${LIB_VULKAN})

add_definitions(-DENABLE_CPP20_MODULE=OFF)

target_compile_definitions(${LIB_VULKAN} PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

target_link_libraries(${LIB_VULKAN} PUBLIC Vulkan::Vulkan)
set_target_properties(${LIB_VULKAN} PROPERTIES CXX_STANDARD 23)

target_sources(${LIB_VULKAN}
        PUBLIC
            FILE_SET CXX_MODULES
            BASE_DIRS ${Vulkan_INCLUDE_DIR}
            FILES "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm" "${Vulkan_INCLUDE_DIR}/vulkan/vulkan_video.cppm"
)