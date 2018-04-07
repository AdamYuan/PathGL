cmake_minimum_required(VERSION 3.7)
project(PathGL)

set(CMAKE_CXX_STANDARD 11)

add_executable(PathGL
		main.cpp
		Application.cpp Application.hpp Config.hpp Camera.cpp Camera.hpp
		include/GL/gl3w.c SceneLoader.cpp SceneLoader.hpp)

add_definitions(-DSTB_IMAGE_WRITE_IMPLEMENTATION)
include_directories(include/)
target_link_libraries(PathGL dl GL glfw)