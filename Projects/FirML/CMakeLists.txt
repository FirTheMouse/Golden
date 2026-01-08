
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm
)
add_executable(FirML ${GAME_SOURCES})

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/methods.mm
    PROPERTIES
    COMPILE_FLAGS "-x objective-c++ -fobjc-arc"
)

target_include_directories(FirML PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/Projects/FirML/include
)                                          

target_compile_options(FirML PRIVATE
    -march=native  # Use the current machine's architecture
)

if(APPLE)
    target_compile_definitions(FirML PRIVATE 
        ACCELERATE_NEW_LAPACK
        ACCELERATE_LAPACK_ILP64)
    target_link_libraries(FirML PRIVATE "-framework Accelerate")
endif()



target_link_libraries(FirML PRIVATE GoldenEngine)