file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.mm
)

add_library(GoldenEngine STATIC ${ENGINE_SOURCES})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(GoldenEngine PRIVATE -march=native -mtune=native)
endif()

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

add_library(eigen INTERFACE)
target_include_directories(eigen INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/eigen-5.0.0
)

if(APPLE)
    target_compile_definitions(GoldenEngine PUBLIC 
        ACCELERATE_NEW_LAPACK
        ACCELERATE_LAPACK_ILP64
    )
    target_link_libraries(GoldenEngine PUBLIC "-framework Accelerate")
    
    # Enable Objective-C++ for any .mm files
    set_source_files_properties(
        ${ENGINE_SOURCES}
        PROPERTIES
        COMPILE_FLAGS "-x objective-c++ -fobjc-arc"
    )
endif()

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
    eigen
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
        "-framework MetalPerformanceShaders"
    )
    set(CMAKE_OSX_ARCHITECTURES "arm64") 
elseif(WIN32)
    target_link_libraries(GoldenEngine PUBLIC opengl32)
elseif(UNIX)
    target_link_libraries(GoldenEngine PUBLIC dl pthread)
endif()