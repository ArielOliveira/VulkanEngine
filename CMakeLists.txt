cmake_minimum_required(VERSION 4.3.1)

set(LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/libs")
set(INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/source")
set(SHADERS_DIR "${CMAKE_CURRENT_LIST_DIR}/shaders")

file(GLOB ALL_SOURCES ${SRC_DIR}/*.cpp)

###### THIS ENABLES C++23 STD MODULES COMPILATION FOR CMAKE VERSION 4.3.1 ######
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD 451f2fe2-a8a2-47c3-bc32-94786d8fc91b)
set(CMAKE_CXX_STDLIB_MODULES_JSON "C:/msys64/ucrt64/lib/libstdc++.modules.json")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_EXTENSIONS OFF)
################################################################################

project(VulkanEngine LANGUAGES CXX VERSION 0.0.1.0)

include_directories(include ${PROJECT_BINARY_DIR})

configure_file(${INCLUDE_DIR}/version.hpp.in ${PROJECT_BINARY_DIR}/version.hpp)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/definitions")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/shaders")

include(LibraryNames)
include(ShaderCompiler)

add_subdirectory(${LIB_DIR}/glfw)
add_subdirectory(${LIB_DIR}/vulkan)

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE glfw Vulkan::cppm)
target_compile_definitions(${PROJECT_NAME} PRIVATE CMAKE_EXPORT_COMPILE_COMMANDS=1)

compile_slang_shaders(SHADERS ${SHADERS_DIR} "${PROJECT_BINARY_DIR}/$<CONFIG>")
add_dependencies(${PROJECT_NAME} SHADERS)