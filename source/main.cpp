#include <vector>
#include <filesystem>
#include <ranges>
#include <iostream>

#include "application_config.hpp"
#include "application.hpp"

int main(int argc, char** argv)
{
    application_config config;

    if (const auto opt = parse_application_config(config, argc, argv); opt.has_value())
        return opt.value();

    try
    {
        application app(config);
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -1;
    }

    return 0;
}
