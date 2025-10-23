
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(AvalVentures ${GAME_SOURCES})

target_include_directories(AvalVentures PRIVATE
${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/AvalVentures/include
)                                          
target_link_libraries(AvalVentures PRIVATE GoldenEngine)