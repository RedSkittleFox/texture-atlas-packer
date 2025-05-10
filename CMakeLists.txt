cmake_minimum_required(VERSION 3.20.0)

project(
    texture-atlas-packer
    LANGUAGES
        C CXX
    VERSION
        1.0.0
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(deps)
add_subdirectory(source)

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-source")
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")

set(CPACK_SOURCE_IGNORE_FILES
    .git/
    .idea/
    build/
)

include(CPack)