cmake_minimum_required (VERSION 3.8)
project ("ForceSensor")

set(CMAKE_CXX_STANDARD 14)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message ("CMAKE_BUILD_TYPE = Debug")
else ()
    set (CMAKE_BUILD_TYPE "Release")
    message ("CMAKE_BUILD_TYPE = Release")
    if (MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O2)
    endif()
endif ()
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
include_directories(${CMAKE_SOURCE_DIR}/include)
add_subdirectory(${CMAKE_SOURCE_DIR}/examples)
