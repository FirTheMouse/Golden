
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(Learning ${GAME_SOURCES})

target_include_directories(Learning PRIVATE
${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/Learning/include
)                                          
target_link_libraries(Learning PRIVATE GoldenEngine)