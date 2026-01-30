
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(TheGolden ${GAME_SOURCES})

target_include_directories(TheGolden PRIVATE
${CMAKE_SOURCE_DIR}/include
)                                          
target_link_libraries(TheGolden PRIVATE GoldenEngine)