#include "application.hpp"

#include <stdexcept>
#include <format>
#include <set>
#include <ranges>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <cassert>
#include <unordered_map>
#include <cstring>
#include <execution>
#include <string>
#include <fstream>

#include <rectpack2D/finders_interface.h>
#include <nlohmann/json.hpp>

#include "image_file_io.hpp"

application::application(application_config& config)
    : config_(config), min_channels_(0)
{
    switch (config_.image_output_format)
    {
    case e_image_output_format::BMP:
    case e_image_output_format::JPG:
        max_channels_ = 3;
        break;
    case e_image_output_format::PNG:
    case e_image_output_format::TGA:
        max_channels_ = 4;
        break;
    default:
        std::unreachable();
    }
}

void application::run()
{
    this->generate_image_database();
    this->pack();
    this->generate_atlases();
    this->write_atlases();
    this->write_config();
}

void application::generate_image_database()
{
    // Used to check if relative paths don't overlap
    std::unordered_map<
        std::filesystem::path, // relative path
        const std::filesystem::path* // absolute path
    > processed_files;

    for (auto& path : config_.source_directories)
    {
        // Don't process duplicates twice
        if (images_.contains(path))
            continue;

        // Check if path is a folder
        if (!std::filesystem::is_directory(path))
            throw std::invalid_argument(std::format("'{}' is not a directory.", path.string()));

        auto [it, b] = images_.emplace(path, std::vector<image>());
        std::ignore = b;

        auto& images = it->second;

        // Used in the processed_files map to optimize memory allocation
        const std::filesystem::path* p_path = std::addressof(it->first);

        // Iterate over files in the directory recursively
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
        {
            if (!std::filesystem::is_regular_file(entry))
                continue;

            // Compute atlas path
            std::filesystem::path atlas_path;
            atlas_path = std::filesystem::relative(entry.path(), path);

            if (config_.config_include_extensions_in_atlas_file_names == false)
            {
                atlas_path = atlas_path.replace_extension();
            }

            atlas_path.make_preferred();

            auto [it, emplaced] = processed_files.try_emplace(atlas_path.string(), p_path);
            if (emplaced == false)
            {
                std::print(
                    std::cerr,
                    "Duplicate atlas paths '{}' in folders '{}' and '{}'.\n\tKeeping '{}'.\n\tSkipping '{}'.\n",
                    std::filesystem::relative(entry.path(), path).string(),
                    it->second->string(),
                    path.string(),
                    it->first.string(),
                    entry.path().string()
                );

                continue;
            }

            auto file = open_file(entry);

            if (file == nullptr)
            {
                std::print(std::cerr, "Failed to open file '{}'. Skipping...\n", entry.path().string());
                continue;
            }

            std::size_t width, height, channels;
            auto result = read_image_metadata(file.get(), width, height, channels);

            if (result.has_value() == false)
            {
                std::print(std::cerr, "Failed to read '{}' as an image. {}. Skipping...\n",
                    entry.path().string(), result.error());
                continue;
            }

            if ( static_cast<std::uint32_t>(channels) > max_channels_)
            {
                std::print(
                    std::cerr, "Selected image format does not support {} channels. Some data will be lost.\n",
                    channels
                    );
            }

            min_channels_ = std::max(min_channels_, std::min(static_cast<std::uint32_t>(channels), max_channels_));
            auto& image = images.emplace_back();
            image.path = std::filesystem::absolute(entry.path());
            image.atlas_path = atlas_path;

            image.width = static_cast<std::uint32_t>(width);
            image.height = static_cast<std::uint32_t>(height);
        }
    }
}

void application::pack()
{
    namespace rp = rectpack2D;

    constexpr auto flipping = rp::flipping_option::DISABLED;

    using spaces_type = rp::empty_spaces<false>;
    using rect_type = spaces_type::output_rect_type;

    // Create rectangles
    std::vector<rect_type> rects;
    for (auto& images : images_ | std::views::values)
    {
        rects.reserve(std::size(rects) + std::size(images));
        std::ranges::transform(
            images, std::back_inserter(rects),
            [](const image& i)
            {
                return rect_type{0, 0, static_cast<int>(i.width), static_cast<int>(i.height)};
            }
        );
    }

    // Uses greedy approach to fill
    std::vector<rect_type> current_bin_rectangles;
    std::vector<rect_type> remaining_rectangles;
    std::vector<std::vector<rect_type>> bins;

    auto report_successful = [&current_bin_rectangles](rect_type& rect)
    {
        current_bin_rectangles.push_back(rect);
        return rp::callback_result::CONTINUE_PACKING;
    };

    auto report_unsuccessful = [&remaining_rectangles](rect_type& rect)
    {
        remaining_rectangles.push_back(rect);
        return rp::callback_result::CONTINUE_PACKING;
    };

    const auto finder_input = rp::make_finder_input(
        static_cast<int>(config_.atlas_pixel_width),
        1,
        report_successful,
        report_unsuccessful,
        rp::flipping_option::DISABLED
    );

    while (!std::empty(rects)) // All rects exhausted
    {
        [[maybe_unused]] auto result = rp::find_best_packing<spaces_type>(rects, finder_input);

        bins.push_back(current_bin_rectangles);
        current_bin_rectangles.clear();
        rects = std::move(remaining_rectangles);
    }

    // Map rects to textures

    // TODO: rectpack2D doesn't provide a way to pass custom data to rects
    // so you have to retroactively match them to the original dataset.
    // Possible one could use std::addressof() on handler lambdas parameters and calculate
    // the offset in the array and map it to original data but I don't think it's
    // guaranteed anywhere in the documentation.

    // Iterate over all bins
    for (auto&& [id, bin] : std::views::enumerate(bins))
    {
        for (auto& rect : bin) // Iterate over all rects
        {
            // Iterate over all images
            for (auto& images : images_ | std::views::values)
            {
                bool found = false;

                for (auto& image : images)
                {
                    // Already assigned a bin
                    if (image.bin != bin_n_pos)
                        continue;

                    // Image and rect are not the same
                    if (image.width != rect.w || image.height != rect.h)
                        continue;

                    // Assign the location to the image
                    image.bin = id;
                    image.x = rect.x;
                    image.y = rect.y;

                    found = true;
                    break;
                }

                if (found)
                    break;
            }
        }
    }

    bins_.resize(std::size(bins));
}

void application::generate_atlases()
{
    // Compute the size per texture
    const std::size_t row_stride = config_.atlas_pixel_width * min_channels_ * sizeof(std::uint8_t);
    const std::size_t bin_stride = row_stride * config_.atlas_pixel_width;

    // Generate zero bitmaps
    for (auto& bin : bins_)
    {
        bin = std::make_unique<std::uint8_t[]>(bin_stride);
        std::memset(bin.get(), 0x0, config_.atlas_pixel_width);
    }

    // TODO: Do parallel-for-each loop
    for (auto& images : images_ | std::views::values)
    {
        std::for_each(
            std::execution::par_unseq,
            std::begin(images),
            std::end(images),
            [&](const image& image) -> void
            {
                auto file = open_file(image.path);

                if (file == nullptr)
                {
                    std::print(std::cerr, "Failed to read file '{}'. Skipping...\n", image.path.string());
                    return;
                }

                std::size_t width, height, channels;
                auto result = read_image(file.get(), min_channels_, width, height, channels);

                if (result.has_value() == false)
                {
                    std::print(
                        std::cerr,
                        "Failed to read image '{}'. {}. Skipping...\n",
                        image.path.string(), result.error()
                    );

                    return;
                }

                if (width != image.width || height != image.height)
                {
                    std::print(
                        std::cerr,
                        "Dimensions of image '{}' have changed. Skipping...\n",
                        image.path.string()
                    );
                    return;
                }

                // TODO: Handle image rotations
                auto image_data = std::move(result.value());

                auto p_dest_image = bins_[image.bin].get();
                auto p_source_image = image_data.get();

                const std::size_t source_image_stride = width * min_channels_ * sizeof(std::uint8_t);

                const std::size_t dest_x_offset = sizeof(std::uint8_t) * image.x * min_channels_;
                for (std::size_t row{}; row < height; ++row)
                {
                    std::uint8_t* p_dest = p_dest_image
                        + row_stride * (image.y + row)
                        + dest_x_offset;

                    const std::uint8_t* p_source = p_source_image + row * source_image_stride;

                    std::memcpy(p_dest, p_source, source_image_stride);
                }
            }
        );
    }
}

void application::write_atlases()
{
    if (!std::filesystem::is_directory(config_.image_output_directory))
    {
        throw std::runtime_error(std::format("Invalid output directory '{}'.", config_.image_output_directory.string()));
    }

    for (std::size_t i = 0; i < std::size(bins_); ++i)
    {
        auto file_name = format_image_file_name(i + 1);
        auto out_path = config_.image_output_directory / file_name;

        if (config_.config_use_bin_image_absolute_path)
        {
            bin_paths_.push_back(out_path);
        }
        else
        {
            bin_paths_.emplace_back(file_name);
        }

        const bool result = write_image(config_.image_output_format, out_path,
            bins_[i].get(),
            min_channels_,
            config_.atlas_pixel_width,
            config_.atlas_pixel_width
        );

        if (result == false)
        {
            throw std::runtime_error(std::format("Failed to write atlas '{}'.", out_path.string()));
        }
    }
}

void application::write_config()
{
    using json = nlohmann::json;

    using std::filesystem::path;

    json j
    {
        {"version", 1},
        { "bin-textures", this->bin_paths_ | std::views::transform([](auto& e) { /*std::mem_fn fails*/ return e.string(); }) | std::ranges::to<std::vector>() }
    };

    for (auto& images : images_ | std::views::values)
    {
        for (auto& image : images)
        {
            auto& json_image = j["images"][image.atlas_path.string()];
            json_image["bin"] = image.bin;
            json_image["x"] = image.x;
            json_image["y"] = image.y;
            json_image["width"] = image.width;
            json_image["height"] = image.height;
        }
    }

    std::ofstream f(config_.config_output_path);
    if (!f.is_open())
        throw std::runtime_error(std::format("Failed to open '{}' for write.", config_.config_output_path.string()));

    f << std::setw(4) << j << std::endl;
}

std::string application::format_image_file_name(std::size_t image) const
{
    auto format = config_.image_output_name_format.c_str();
    const auto l = std::snprintf(nullptr, 0, format, static_cast<int>(image));

    if (l < 0)
        throw std::invalid_argument(std::format("Format '{}' is invalid.\n", format));

    std::string file_name(l + 1, ' ');
    std::snprintf(std::data(file_name), std::size(file_name), format, static_cast<int>(image));

    return static_cast<std::string>(file_name.c_str()); // Get rid of padding
}




