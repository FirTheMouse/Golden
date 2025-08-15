file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(Golden_alpha ${GAME_SOURCES})

target_include_directories(Golden_alpha PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/Golden_alpha/include
)

target_link_libraries(Golden_alpha PRIVATE GoldenEngine)