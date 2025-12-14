cmake_minimum_required(VERSION 3.29)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_COMPILER clang++)

find_package(Vulkan REQUIRED)
project(bulkin LANGUAGES CXX)

add_subdirectory("third-party/glm")
add_subdirectory("third-party/SDL")
add_subdirectory("third-party/vk-bootstrap")
add_subdirectory("third-party/vma")
add_subdirectory("third-party/volk")
add_subdirectory("third-party/fastgltf")

file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE MY_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

add_executable(bulkin "${MY_SOURCES}" "${MY_HEADERS}")
target_include_directories(bulkin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")

if(APPLE)
  find_library(Cocoa Cocoa REQUIRED)
  find_library(IOKit IOKit REQUIRED)
  find_library(CoreVideo CoreVideo REQUIRED)
  target_link_libraries(bulkin PRIVATE ${Cocoa} ${IOKit} ${CoreVideo})
endif()

add_library(imgui STATIC)

target_include_directories(imgui PUBLIC "third-party/imgui")

target_sources(imgui PRIVATE
  "third-party/imgui/imgui.h"
  "third-party/imgui/imgui.cpp"

  "third-party/imgui/imgui_demo.cpp"
  "third-party/imgui/imgui_draw.cpp"
  "third-party/imgui/imgui_widgets.cpp"
  "third-party/imgui/imgui_tables.cpp"
  "third-party/imgui/backends/imgui_impl_vulkan.cpp"
  "third-party/imgui/backends/imgui_impl_sdl3.cpp"
)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL3::SDL3)

target_link_libraries(bulkin PRIVATE Vulkan::Vulkan SDL3::SDL3 glm vk-bootstrap::vk-bootstrap VulkanMemoryAllocator volk_headers imgui fastgltf)
target_compile_options(bulkin PRIVATE -Wall -Wextra -g)
