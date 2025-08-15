
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
add_executable(StudentSystem ${GAME_SOURCES})

target_include_directories(StudentSystem PRIVATE
${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/StudentSystem/include
)                                          
target_link_libraries(StudentSystem PRIVATE GoldenEngine)