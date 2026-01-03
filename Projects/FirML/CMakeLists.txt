
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(FirML ${GAME_SOURCES})

target_include_directories(FirML PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/Projects/FirML/include
)                                          
target_link_libraries(FirML PRIVATE GoldenEngine)