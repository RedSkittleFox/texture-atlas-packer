#pragma once

#include <cstdint>
#include <vector>
#include <filesystem>
#include <optional>

enum class e_image_output_format
{
    PNG,
    BMP,
    TGA,
    JPG
};

enum class e_config_output_format
{
    JSON
};

struct application_config
{
    std::vector<std::filesystem::path> source_directories;

    // TODO:
    /*
    struct image_path
    {
        std::filesystem::path source;
        std::filesystem::path destination;
    };
    std::vector<image_path> source_images;
    */

    std::uint32_t atlas_pixel_width;

    std::filesystem::path image_output_directory;
    std::string image_output_name_format;
    e_image_output_format image_output_format;

    std::filesystem::path config_output_path;
    bool config_use_bin_image_absolute_path;
    bool config_include_extensions_in_atlas_file_names;
    e_config_output_format config_output_format;
};

std::optional<int> parse_application_config(application_config& config, int argc, char** argv);
