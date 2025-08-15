file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(Testing ${GAME_SOURCES})

target_include_directories(Testing PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/Testing/include
)

target_link_libraries(Testing PRIVATE GoldenEngine)