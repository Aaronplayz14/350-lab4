cmake_minimum_required(VERSION 3.16)
project(cache LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB EXE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

foreach(SOURCE IN LISTS EXE)
    get_filename_component(TARGET_NAME "${SOURCE}" NAME_WE)
    add_executable(${TARGET_NAME} "${SOURCE}")

    target_compile_features(${TARGET_NAME} PUBLIC cxx_std_20)
    if (MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4 /permissive- /Zc:__cplusplus)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang|GNU)$")
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif ()
endforeach()
