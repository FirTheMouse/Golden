file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(Fshgame ${GAME_SOURCES})

target_include_directories(Fshgame PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/Fshgame/include
)

target_link_libraries(Fshgame PRIVATE GoldenEngine)