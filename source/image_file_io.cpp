#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "image_file_io.hpp"

void file_deleter::operator()(std::FILE* file_ptr) const noexcept
{
    if (file_ptr)
        std::fclose(file_ptr);
}

std::unique_ptr<std::FILE, file_deleter> open_file(const std::filesystem::path& path, bool read)
{
    std::unique_ptr<std::FILE, file_deleter> file;

#ifdef _WIN32
    file.reset(_wfopen(path.wstring().c_str(), read ? L"rb" : L"wb+"));
#else
    file.reset(std::fopen(path.string().c_str(), read ? "rb" : "wb+"));
#endif

    return file;
}

std::expected<void, std::string> read_image_metadata(FILE* file, std::size_t& width, std::size_t& height, std::size_t& channels)
{
    int ix, iy, ichannels;
    if (stbi_info_from_file(file, std::addressof(ix), std::addressof(iy), std::addressof(ichannels)) == 0)
    {
        return std::unexpected(static_cast<std::string>(stbi_failure_reason()));
    }

    width = static_cast<std::size_t>(ix);
    height = static_cast<std::size_t>(iy);
    channels = static_cast<std::size_t>(ichannels);

    return {};
}

void stbi_image_deleter::operator()(std::uint8_t* data_ptr) const noexcept
{
    if (data_ptr)
        stbi_image_free(data_ptr);
}

std::expected<std::unique_ptr<std::uint8_t, stbi_image_deleter>, std::string>
    read_image(FILE* file, std::size_t requested_channels, std::size_t& width, std::size_t& height, std::size_t& channels)
{
    std::unique_ptr<std::uint8_t, stbi_image_deleter> data;

    int ix, iy, ichannels;
    data.reset(stbi_load_from_file(
        file,
        std::addressof(ix),
        std::addressof(iy),
        std::addressof(ichannels),
        static_cast<int>(requested_channels)
    ));

    if (data == nullptr)
    {
        return std::unexpected(static_cast<std::string>(stbi_failure_reason()));
    }

    width = static_cast<std::size_t>(ix);
    height = static_cast<std::size_t>(iy);
    channels = static_cast<std::size_t>(ichannels);

    return data;
}

bool write_image(
    e_image_output_format format,
    const std::filesystem::path& path,
    void* data,
    std::uint32_t channels,
    std::size_t width,
    std::size_t height
    )
{
    auto path_string = path.string();
    auto path_c_string = path_string.c_str();

    switch (format)
    {
    case e_image_output_format::PNG:
        return stbi_write_png(
            path_c_string,
                static_cast<int>(width),
                static_cast<int>(height),
                static_cast<int>(channels),
                data,
                static_cast<int>(channels) * width * sizeof(std::uint8_t)
            );
    case e_image_output_format::BMP:
        return stbi_write_bmp(
            path_c_string,
            static_cast<int>(width),
            static_cast<int>(height),
            static_cast<int>(channels),
            data
        );
    case e_image_output_format::TGA:
        return stbi_write_tga(
        path_c_string,
        static_cast<int>(width),
        static_cast<int>(height),
        static_cast<int>(channels),
        data
        );
    case e_image_output_format::JPG:
        return stbi_write_jpg(
            path_c_string,
            static_cast<int>(width),
            static_cast<int>(height),
            static_cast<int>(channels),
            data,
            100
        );
    default:
        std::unreachable();
    }
}