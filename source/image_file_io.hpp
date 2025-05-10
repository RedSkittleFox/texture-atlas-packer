#pragma once

#include <memory>
#include <cstdio>
#include <filesystem>
#include <expected>

#include "application_config.hpp"

struct file_deleter
{
    void operator()(std::FILE* file_ptr) const noexcept;
};

std::unique_ptr<std::FILE, file_deleter> open_file(const std::filesystem::path& path, bool read = true);

std::expected<void, std::string> read_image_metadata(FILE* file, std::size_t& width, std::size_t& height, std::size_t& channels);

struct stbi_image_deleter
{
    void operator()(std::uint8_t* data_ptr) const noexcept;
};

std::expected<std::unique_ptr<std::uint8_t, stbi_image_deleter>, std::string>
read_image(FILE* file, std::size_t requested_channels, std::size_t& width, std::size_t& height, std::size_t& channels);

bool write_image(
    e_image_output_format format,
    const std::filesystem::path& path,
    void* data,
    std::uint32_t channels,
    std::size_t width,
    std::size_t height
);