cmake_minimum_required(VERSION 3.9)
project(PathGL)

set(CMAKE_CXX_STANDARD 11)

add_executable(PathGL
        main.cpp
        Application.cpp Application.hpp
        include/GL/gl3w.c Config.hpp Camera.cpp Camera.hpp)

add_definitions(-DSTB_IMAGE_WRITE_IMPLEMENTATION)
include_directories(include/)
target_link_libraries(PathGL dl GL glfw)