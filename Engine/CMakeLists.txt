file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_library(GoldenEngine STATIC ${ENGINE_SOURCES})
target_compile_definitions(GoldenEngine PUBLIC GLFW_INCLUDE_NONE)

target_include_directories(GoldenEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/glm
)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb
)

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/json/include
)

add_library(tinygltf INTERFACE)
target_include_directories(tinygltf INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinygltf
)
target_link_libraries(tinygltf INTERFACE stb)

target_link_libraries(GoldenEngine PUBLIC
    glm
    stb
    nlohmann_json
    tinygltf
)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" )
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" )
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" )

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw-3.4)

target_link_libraries(GoldenEngine PUBLIC glfw)

target_sources(GoldenEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/src/glad.c
)
target_include_directories(GoldenEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/glad/include
)

if(APPLE)
    target_link_libraries(GoldenEngine PUBLIC
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework QuartzCore"
        "-framework Metal"
    )
elseif(WIN32)
    target_link_libraries(GoldenEngine PUBLIC opengl32)
elseif(UNIX)
    target_link_libraries(GoldenEngine PUBLIC dl pthread)
endif()