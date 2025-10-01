cmake_minimum_required(VERSION 3.29)

find_package(Vulkan REQUIRED)
project(bulkin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE MY_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

add_library(bulkin "${MY_SOURCES}" "${MY_HEADERS}" "${MY_SHADERS}")
target_include_directories(bulkin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")

if(APPLE)
  find_library(Cocoa Cocoa REQUIRED)
  find_library(IOKit IOKit REQUIRED)
  find_library(CoreVideo CoreVideo REQUIRED)
  target_link_libraries(bulkin PRIVATE ${Cocoa} ${IOKit} ${CoreVideo})
endif()

target_link_libraries(bulkin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdParty/libglfw3.a)
target_link_libraries(bulkin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdParty/libtmxlite.a)
target_link_libraries(bulkin PRIVATE Vulkan::Vulkan)
target_compile_options(bulkin PRIVATE -Wall -Wextra -g)
