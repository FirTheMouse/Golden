
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(ThymeLoop ${GAME_SOURCES})

target_include_directories(ThymeLoop PRIVATE
${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/ThymeLoop/include
)                                          
target_link_libraries(ThymeLoop PRIVATE GoldenEngine)