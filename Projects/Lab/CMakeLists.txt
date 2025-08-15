
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(Lab ${GAME_SOURCES})

target_include_directories(Lab PRIVATE
${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/Lab/include
)                                          
target_link_libraries(Lab PRIVATE GoldenEngine)