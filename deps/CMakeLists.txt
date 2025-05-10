function(register_license PROJECT VERSION LICENSE_FILE)
    list(APPEND LICENSE_PROJECT_LIST "${PROJECT}")
    list(APPEND LICENSE_VERSION_LIST "${VERSION}")
    list(APPEND LICENSE_FILE_LIST "${LICENSE_FILE}")

    set(LICENSE_PROJECT_LIST "${LICENSE_PROJECT_LIST}" PARENT_SCOPE)
    set(LICENSE_VERSION_LIST "${LICENSE_VERSION_LIST}" PARENT_SCOPE)
    set(LICENSE_FILE_LIST "${LICENSE_FILE_LIST}" PARENT_SCOPE)
endfunction()

include(FetchContent)

FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG 4160d259d961cd393fd8d67590a8c7d210207348 # v2.5.0
)

FetchContent_Declare(
    rectpack2D
    GIT_REPOSITORY https://github.com/TeamHypersomnia/rectpack2D.git
    GIT_TAG 3344bf5f39db1a9fb45fd82bfc85c3a5be045e3b # Commit from May 3rd 2025
)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG f0569113c93ad095470c54bf34a17b36646bbbb5 # Commit from Mar 14 2025
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG 55f93686c01528224f448c19128836e7df245f72 # v3.12.0
)

FetchContent_MakeAvailable(CLI11 rectpack2D stb nlohmann_json)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
add_library(stb::stb ALIAS stb)

register_license("CLI11" "v2.5.0" "${CLI11_SOURCE_DIR}/LICENSE")
register_license("rectpack2D" "commit 3344bf5f39db1a9fb45fd82bfc85c3a5be045e3b" "${rectpack2D_SOURCE_DIR}/LICENSE")
register_license("stb" "commit f0569113c93ad095470c54bf34a17b36646bbbb5" "${stb_SOURCE_DIR}/LICENSE")
register_license("nlohmann_json" "v3.12.0" "${nlohmann_json_SOURCE_DIR}/LICENSE.MIT")

# Generate the combined third party license file
set(THIRD_PARTY_LICENSES_FILE "${CMAKE_BINARY_DIR}/THIRD_PARTY_LICENSES.txt")

# Generate empty file
file(WRITE "${THIRD_PARTY_LICENSES_FILE}")

list(LENGTH LICENSE_PROJECT_LIST num_licenses)
math(EXPR license_range_to "${num_licenses} - 1")

foreach(i RANGE 0 ${license_range_to})
    list(GET LICENSE_PROJECT_LIST ${i} license_project)
    list(GET LICENSE_VERSION_LIST ${i} license_version)
    list(GET LICENSE_FILE_LIST ${i} license_file)

    file(READ "${license_file}" license_contents)

    if("${license_contents}" STREQUAL "")
        message(FATAL_ERROR "Failed to read a license file for project ${license_project}. File '${license_file}'.")
    endif()

    file(APPEND "${THIRD_PARTY_LICENSES_FILE}"
        "\n"
        "============================================================================\n\n"
        "Package: ${license_project}\n"
        "Version: ${license_version}\n"
        "License:\n\n"
        "${license_contents}"
    )
endforeach()

# Install licenses
install(FILES ${PROJECT_SOURCE_DIR}/LICENSE ${THIRD_PARTY_LICENSES_FILE} DESTINATION "share/licenses")