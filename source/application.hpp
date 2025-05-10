#pragma once

#include <vector>
#include <filesystem>
#include <unordered_map>

#include "application_config.hpp"

class application
{
    static constexpr std::uint32_t bin_n_pos = std::numeric_limits<std::uint32_t>::max();
    application_config& config_;

    struct image
    {
        std::filesystem::path path;
        std::filesystem::path atlas_path;

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
    std::uint32_t max_channels_ = 0;

    std::vector<std::filesystem::path> bin_paths_;
    std::vector<std::unique_ptr<std::uint8_t[]>> bins_;

public:
    application() = delete;
    application(const application&) = delete;
    application(application&&) noexcept = delete;
    application& operator=(const application&) = delete;
    application& operator=(application&&) noexcept = delete;
    ~application() noexcept = default;

public:
    explicit application(application_config& config);
    void run();

private:
    void generate_image_database();
    void pack();
    void generate_atlases();
    void write_atlases();
    void write_config();

    std::string format_image_file_name(std::size_t image) const;
};
