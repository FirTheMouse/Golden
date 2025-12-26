file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_library(GoldenEngine STATIC ${ENGINE_SOURCES})

target_include_directories(GoldenEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ext
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/glad
)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
  set(GLFW_LIB "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/lib-arm64/libglfw3.a")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(GLFW_LIB "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/lib-x86_64/libglfw3.a")
else()
  message(FATAL_ERROR "Unknown processor: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Example: Link GLFW or other libraries here
target_link_libraries(GoldenEngine PUBLIC
    ${GLFW_LIB}
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
    "-framework QuartzCore"
    "-framework Metal"
)