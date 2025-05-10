#include <CLI/CLI.hpp>
#include <map>
#include <string>

#include "application_config.hpp"

const std::map<std::string, e_image_output_format> output_image_format_map
{
       {"png", e_image_output_format::PNG},
       {"bmp", e_image_output_format::BMP},
       {"tga", e_image_output_format::TGA},
        {"jpg", e_image_output_format::JPG}
};

const std::map<std::string, e_config_output_format> output_config_format_map
{
    {"json", e_config_output_format::JSON}
};

std::optional<int> parse_application_config(application_config& config, int argc, char** argv)
{
    CLI::App app("texture-atlas-packer is an utility for packing folders of textures into texture atlases.");

    app.add_option("-d,--directory", config.source_directories,
        "Directories to include in texture atlas.")
        ->expected(1, -1)
        ->take_all()
        ->multi_option_policy(CLI::MultiOptionPolicy::TakeAll);

    app.add_option("-s,--size", config.atlas_pixel_width,
        "Atlas's texture size (SxS), default is 1024.")
        ->default_val(1024);

    app.add_option("-o,--image-output-directory", config.image_output_directory,
        "Image output directory.")
        ->default_val("./");

    app.add_option("--image-output-name-format", config.image_output_name_format,
        "Image name format (atlas-%02).png") // TODO: Update this to atlas-{{index}}.{{ext}}
        ->default_val("atlas-%02d.png");

    app.add_option("--image-output-format", config.image_output_format,
        "Output image format (png, bmp, tga, jpg), default is png.")
        ->transform(CLI::CheckedTransformer(output_image_format_map, CLI::ignore_case))
        ->default_val(e_image_output_format::PNG);

    app.add_option("-c,--config-output-path", config.config_output_path,
        "Config output path, default is ./config.json")
        ->default_val("./config.json");

    app.add_option("--config-use-bin-image-absolute-path", config.config_use_bin_image_absolute_path,
        "Use absolute image paths for bin images. Default is false.")
        ->default_val(false);

    app.add_option("--config-include-extensions-in-atlas-file-names", config.config_include_extensions_in_atlas_file_names,
        "Include extensions in the atlas images file names in the config file. Default is false.")
        ->default_val(false);

    app.add_option("--config-output-format", config.config_output_format,
        "Output config format (json), default is json.")
        ->transform(CLI::CheckedTransformer(output_config_format_map, CLI::ignore_case))
        ->default_val(e_config_output_format::JSON);

    CLI11_PARSE(app, argc, argv);

    return std::nullopt;
}