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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <rectpack2D/finders_interface.h>

application::application(const std::vector<std::filesystem::path>& paths, std::uint32_t bin_size)
    : min_channels_(0), bin_size_(bin_size)
{
    const std::set<std::string> supported_extensions
    {
        ".png", ".jpg", ".bmp", ".psd", ".gif", ".hdr", ".pic", ".pnm", ".ppm", ".pgm"
    };

    // Used to check if relative paths don't overlap
    std::unordered_map<
        std::filesystem::path, // relative path
        const std::filesystem::path* // absolute path
    > processed_files;

    for (auto& path : paths)
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

            auto [it, emplaced] = processed_files.try_emplace(entry.path(), p_path);
            if (emplaced == false)
            {
                std::print(
                    std::cerr,
                    "Duplicate relative paths '{}' in folders '{}' and '{}'.\n\tKeeping '{}'.\n\tSkipping '{}'.\n",
                    std::filesystem::relative(entry.path(), path).string(),
                    it->second->string(),
                    path.string(),
                    it->first.string(),
                    entry.path().string()
                );

                continue;
            }

            std::string extension = entry.path().extension().string();
            std::ranges::for_each(extension, [](char& c) { c = std::tolower(c); });

            if (!supported_extensions.contains(extension))
                continue;

            std::unique_ptr<std::FILE, decltype([](FILE* f) { if (f) std::fclose(f); } )> file;

#ifdef _WIN32
                file.reset(_wfopen(entry.path().wstring().c_str(), L"rb"));
#else
            file.reset(std::fopen(entry.path().string().c_str(), "rb"));
#endif

            if (file.get() == nullptr)
            {
                std::print(std::cerr, "Failed to open file '{}'. Skipping...\n", entry.path().string());
                continue;
            }

            int x, y, channels;
            if (stbi_info_from_file(file.get(), std::addressof(x), std::addressof(y), std::addressof(channels)) == 0)
            {
                std::print(
                    std::cerr, "Failed to load image '{}': {}. Skipping...\n",
                    entry.path().string(), stbi_failure_reason()
                );

                continue;
            }

            min_channels_ = std::max(min_channels_, static_cast<std::uint32_t>(channels));
            auto& image = images.emplace_back();
            image.path = entry.path();
            image.width = static_cast<std::uint32_t>(x);
            image.height = static_cast<std::uint32_t>(y);
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
        bin_size_,
        1,
        report_successful,
        report_unsuccessful,
        rp::flipping_option::DISABLED
    );

    while (!std::empty(rects)) // All rects exhausted
    {
        auto result = rp::find_best_packing<spaces_type>(rects, finder_input);

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
    const std::size_t row_stride = bin_size_ * min_channels_ * sizeof(std::uint8_t);
    const std::size_t bin_stride = row_stride * bin_size_;

    // Generate zero bitmaps
    for (auto& bin : bins_)
    {
        bin = std::make_unique<std::uint8_t[]>(bin_stride);
        std::memset(bin.get(), 0x0, bin_size_);
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
                std::unique_ptr<std::FILE, decltype([](FILE* f) { if (f) std::fclose(f); })> file;

#ifdef _WIN32
                file.reset(_wfopen(image.path.wstring().c_str(), L"rb"));
#else
                file.reset(std::fopen(image.path.string().c_str(), "rb"));
#endif

                if (file.get() == nullptr)
                {
                    std::print(std::cerr, "Failed to read file '{}'. Skipping...\n", image.path.string());
                    return;
                }

                int width, height, channels;
                std::unique_ptr<std::uint8_t, decltype([](std::uint8_t* ptr) { if (ptr) stbi_image_free(ptr); } )>
                    image_data;

                image_data.reset(
                    stbi_load_from_file(
                        file.get(),
                        std::addressof(width),
                        std::addressof(height),
                        std::addressof(channels),
                        this->min_channels_
                    )
                );

                if (image_data.get() == nullptr)
                {
                    std::print(
                        std::cerr,
                        "Failed to read image '{}'. {}. Skipping...\n",
                        image.path.string(), stbi_failure_reason()
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

void application::write_atlases(output_image_format format, const std::filesystem::path& output_folder, const std::string& output_name_format)
{
    for (std::size_t i = 0; i < std::size(bins_); ++i)
    {
        const auto l = std::snprintf(nullptr, 0, output_name_format.c_str(), static_cast<int>(i));

        if (l < 0)
            throw std::invalid_argument(std::format("Format '{}' is invalid.\n", output_name_format));

        std::string file_name(l + 1, ' ');
        std::snprintf(std::data(file_name), std::size(file_name), output_name_format.c_str(), static_cast<int>(i + 1));

        auto out_path = output_folder / file_name;

        bool result = false;

        switch (format)
        {
        case output_image_format::PNG:
            result = stbi_write_png(
                out_path.string().c_str(),
                    bin_size_,
                    bin_size_,
                    min_channels_,
                    bins_[i].get(),
                    min_channels_ * bin_size_ * sizeof(std::uint8_t)
                );
            break;
        case output_image_format::BMP:
            result = stbi_write_bmp(
                out_path.string().c_str(),
                bin_size_,
                bin_size_,
                min_channels_,
                bins_[i].get()
            );
            break;

        case output_image_format::TGA:
            result = stbi_write_tga(
                out_path.string().c_str(),
                bin_size_,
                bin_size_,
                min_channels_,
                bins_[i].get()
            );
            break;

        case output_image_format::JPG:
            result = stbi_write_jpg(
                out_path.string().c_str(),
                bin_size_,
                bin_size_,
                min_channels_,
                bins_[i].get(),
                100
            );
            break;
        default:
            std::unreachable();
        }

        if (result == false)
        {
            throw std::runtime_error(std::format("Failed to write atlas '{}'.", out_path.string()));
        }
    }
}




