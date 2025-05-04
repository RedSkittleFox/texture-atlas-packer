#include <CLI/CLI.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <ranges>

#include "application.hpp"

int main(int argc, char** argv)
{
    CLI::App app("texture-atlas-packer is an utility for packing folders of textures into texture atlases.");

    std::vector<std::string> directories;
    app.add_option("-d,--directory", directories, "Directories to include in texture atlas.")
        ->expected(1, -1)
        ->take_all()
        ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll);

    std::string output_folder;
    app.add_option("-o,--output", output_folder, "Output Folder")
        ->default_val("./");

    std::uint32_t size;
    app.add_option("-s,--size", size, "Atlas's Texture Size will be SxS")
        ->default_val(1024);

    std::string image_format;
    app.add_option("-f,--format", image_format, "Output image format (png, bmp, tga, jpg)")
        ->check(CLI::IsMember({"png", "bmp", "tga", "jpg"}))
        ->default_val("png");

    std::string image_name_format;
    app.add_option("--image-name-format", image_name_format, "Image name format (atlas-%02)")
        ->default_val("atlas-%02d");

    CLI11_PARSE(app, argc, argv);

    application db(
        directories | std::ranges::to<std::vector<std::filesystem::path>>(),
        size
    );

    db.pack();
    db.generate_atlases();
    db.write_atlases(output_format_map.at(image_format), output_folder, image_name_format);
    return 0;
}
