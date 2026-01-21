
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(Emergence ${GAME_SOURCES})

target_include_directories(Emergence PRIVATE
${CMAKE_SOURCE_DIR}/include
)                                          
target_link_libraries(Emergence PRIVATE GoldenEngine)