file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)
#MACOSX_BUNDLE
add_executable(Lab ${GAME_SOURCES})

target_include_directories(Lab PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/include
    ${CMAKE_SOURCE_DIR}/Projects/Lab/include
)

target_link_libraries(Lab PRIVATE GoldenEngine)

#set_target_properties(Lab PROPERTIES
#  MACOSX_BUNDLE_GUI_IDENTIFIER "com.jackson.golden.lab"
#  MACOSX_BUNDLE_BUNDLE_NAME "Lab"
#  MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
#  MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
#)
