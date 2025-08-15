
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(ThreadTest ${GAME_SOURCES})

target_include_directories(ThreadTest PRIVATE
${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/ThreadTest/include
)                                          
target_link_libraries(ThreadTest PRIVATE GoldenEngine)