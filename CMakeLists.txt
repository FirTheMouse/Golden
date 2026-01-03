cmake_minimum_required(VERSION 3.16)
project(Golden LANGUAGES C CXX)

# Set C++ version globally
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(GL_SILENCE_DEPRECATION)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")

set(_GOLDEN_BIN_DIR "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${_GOLDEN_BIN_DIR}")

# For multi-config generators (Visual Studio, Xcode), force every config into the same bin dir
if(CMAKE_CONFIGURATION_TYPES)
  foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${cfg}" cfg_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg_upper} "${_GOLDEN_BIN_DIR}")
  endforeach()
endif()

add_subdirectory(Engine)
add_subdirectory(Projects/GPM)

# ACTIVE_PROJECT_HERE
add_subdirectory(Projects/FirML)



