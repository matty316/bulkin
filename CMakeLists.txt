cmake_minimum_required(VERSION 3.29)

find_package(Vulkan REQUIRED)
project(bulkin LANGUAGES CXX)

set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")

add_subdirectory("thirdParty/glfw")
add_subdirectory("thirdParty/assimp")
add_subdirectory("thirdParty/tmxlite")

set(CMAKE_CXX_STANDARD 23)
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE MY_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

add_library(bulkin "${MY_SOURCES}" "${MY_HEADERS}")
target_include_directories(bulkin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")

if(APPLE)
  find_library(Cocoa Cocoa REQUIRED)
  find_library(IOKit IOKit REQUIRED)
  find_library(CoreVideo CoreVideo REQUIRED)
  target_link_libraries(bulkin PRIVATE ${Cocoa} ${IOKit} ${CoreVideo})
endif()

target_link_libraries(bulkin PRIVATE Vulkan::Vulkan glfw assimp tmxlite)
target_compile_options(bulkin PRIVATE -Wall -Wextra -g)
