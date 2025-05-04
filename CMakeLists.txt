cmake_minimum_required(VERSION 3.20.0)

project(
    texture-atlas-packer
    LANGUAGES
        C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(deps)
add_subdirectory(source)