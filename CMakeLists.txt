cmake_minimum_required(VERSION 3.16)
project(Golden LANGUAGES C CXX)

# Set C++ version globally
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(GL_SILENCE_DEPRECATION)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(Engine)
add_subdirectory(Projects/GPM)

# ACTIVE_PROJECT_HERE
add_subdirectory(Projects/Spaced)



