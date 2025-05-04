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

FetchContent_MakeAvailable(CLI11 rectpack2D stb)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
add_library(stb::stb ALIAS stb)