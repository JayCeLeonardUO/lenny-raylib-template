cmake_minimum_required(VERSION 3.24...3.30)
project(lenny-raylib-template C CXX)

include(FetchContent)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
set(RAYLIB_VERSION 6.0)

FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(raylib)

# Vendored dependencies (git submodules)
add_subdirectory(vendor/box3d)

# Dear ImGui via cimgui (C bindings) + rlImGui (raylib backend).
# cimgui bundles the imgui version its generated C API matches, so that
# copy is the one true imgui in this build. C files get the cimgui structs
# by defining CIMGUI_DEFINE_ENUMS_AND_STRUCTS before #include "cimgui.h".
add_library(imgui STATIC
    vendor/cimgui/cimgui.cpp
    vendor/cimgui/imgui/imgui.cpp
    vendor/cimgui/imgui/imgui_draw.cpp
    vendor/cimgui/imgui/imgui_tables.cpp
    vendor/cimgui/imgui/imgui_widgets.cpp
    vendor/cimgui/imgui/imgui_demo.cpp
)
target_include_directories(imgui PUBLIC vendor/cimgui vendor/cimgui/imgui)

add_library(rlimgui STATIC vendor/rlImGui/rlImGui.cpp)
target_include_directories(rlimgui PUBLIC vendor/rlImGui)
target_link_libraries(rlimgui PUBLIC imgui raylib)

# mylibs unity build: amalgamate mylibs/*.h + *.c into mylibs/mylib.h
# (stb-style, committed so the plain Makefile/CI builds need no codegen)
file(GLOB MYLIBS_FILES CONFIGURE_DEPENDS mylibs/*.h mylibs/*.c)
list(REMOVE_ITEM MYLIBS_FILES ${CMAKE_SOURCE_DIR}/mylibs/mylib.h)
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/mylibs/mylib.h
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/mylibs/amalgamate.cmake
    DEPENDS ${MYLIBS_FILES} mylibs/amalgamate.cmake
    COMMENT "Amalgamating mylibs/ into mylib.h"
)
add_custom_target(mylib_header DEPENDS ${CMAKE_SOURCE_DIR}/mylibs/mylib.h)

# Our Project
add_executable(${PROJECT_NAME})
add_subdirectory(src)
add_dependencies(${PROJECT_NAME} mylib_header)
target_include_directories(${PROJECT_NAME} PRIVATE mylibs)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)

if ("${PLATFORM}" STREQUAL "Web")
    add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/../resources
    )
    #DEPENDS ${PROJECT_NAME}
else()
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    )
    #DEPENDS ${PROJECT_NAME}
endif()

#set(raylib_VERBOSE 1)
target_link_libraries(${PROJECT_NAME} raylib box3d rlimgui)

# Web Configurations
if ("${PLATFORM}" STREQUAL "Web")
    # Tell Emscripten to build an example.html file.
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
    target_link_options(${PROJECT_NAME} PUBLIC -sUSE_GLFW=3 PUBLIC --preload-file resources)
endif()

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
