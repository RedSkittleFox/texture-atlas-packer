add_executable(texture-atlas-packer)

target_sources(
    texture-atlas-packer
    PUBLIC
        main.cpp
        application.cpp
)

target_link_libraries(
    texture-atlas-packer
    PUBLIC
        CLI11::CLI11
        stb::stb
        rectpack2D::rectpack2D
)