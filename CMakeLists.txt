cmake_minimum_required(VERSION 3.16)
project(Golden)

add_compile_definitions(GL_SILENCE_DEPRECATION)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")

# Set C++ version globally
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# Add engine and all games as subdirectories
add_subdirectory(Engine)
add_subdirectory(Projects/GPM)

# ACTIVE_PROJECT_HERE
add_subdirectory(Projects/AvalVentures)

