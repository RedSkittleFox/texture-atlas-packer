add_executable(texture-atlas-packer)

target_sources(
    texture-atlas-packer
    PUBLIC
        main.cpp
        application.cpp
        application_config.cpp
        image_file_io.cpp
)

target_link_libraries(
    texture-atlas-packer
    PUBLIC
        CLI11::CLI11
        stb::stb
        rectpack2D::rectpack2D
        nlohmann_json::nlohmann_json
)

install(TARGETS texture-atlas-packer DESTINATION bin)