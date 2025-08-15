file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(Dueling ${GAME_SOURCES})

#target_compile_options(Dueling PRIVATE -fsanitize=address -g -O3)
#target_link_options(Dueling PRIVATE -fsanitize=address)

target_include_directories(Dueling PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/Dueling/include
)

target_link_libraries(Dueling PRIVATE GoldenEngine)