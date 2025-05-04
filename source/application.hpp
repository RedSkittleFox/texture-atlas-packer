#pragma once

#include <vector>
#include <filesystem>
#include <unordered_map>
#include <map>

enum class output_image_format
{
    PNG,
    BMP,
    TGA,
    JPG
};

const std::map<std::string, output_image_format> output_format_map {
    {"png", output_image_format::PNG},
    {"bmp", output_image_format::BMP},
    {"tga", output_image_format::TGA},
    {"jpg", output_image_format::JPG}
};

class application
{
    static constexpr std::uint32_t bin_n_pos = std::numeric_limits<std::uint32_t>::max();

    struct image
    {
        std::filesystem::path path;

        std::uint32_t width = 0;
        std::uint32_t height = 0;

        std::uint32_t x = 0;
        std::uint32_t y = 0;
        std::uint32_t bin = bin_n_pos;
    };

    std::unordered_map<
        std::filesystem::path, // base path
        std::vector<image> // images in that folder
    > images_;

    std::uint32_t min_channels_ = 3;
    std::uint32_t bin_size_;

    std::vector<std::unique_ptr<std::uint8_t[]>> bins_;

public:
    application() = delete;
    application(const application&) = default;
    application(application&&) noexcept = default;
    application& operator=(const application&) = default;
    application& operator=(application&&) noexcept = default;
    ~application() noexcept = default;

public:
    explicit application(const std::vector<std::filesystem::path>& paths, std::uint32_t bin_size);

    void pack();
    void generate_atlases();
    void write_atlases(output_image_format format, const std::filesystem::path& output_folder, const std::string& output_name_format);
};
