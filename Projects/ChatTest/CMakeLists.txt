
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(ChatTest ${GAME_SOURCES})

target_include_directories(ChatTest PRIVATE
${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/ChatTest/include
)                                          
target_link_libraries(ChatTest PRIVATE GoldenEngine)