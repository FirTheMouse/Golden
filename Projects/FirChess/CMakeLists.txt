
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(FirChess ${GAME_SOURCES})

target_include_directories(FirChess PRIVATE
${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/FirChess/include
)                                          
target_link_libraries(FirChess PRIVATE GoldenEngine)