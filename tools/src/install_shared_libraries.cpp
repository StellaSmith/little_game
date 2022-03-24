#include <argparse/argparse.hpp>
#include <ctre.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <filesystem>
#include <fstream>
#include <ranges>
#include <string>
#include <vector>

using namespace std::literals;

bool g_verbose = true;

static std::vector<std::filesystem::path> libdirs(std::filesystem::path const &path)
{
    std::vector<std::filesystem::path> result;
    std::string section;

    std::ifstream conanbuildinfo(path, std::ios_base::in);
    for (std::string line; std::getline(conanbuildinfo, line);) {
        if (auto match = ctre::match<R"(^\[([a-zA-Z0-9_]+)\]$)">(line)) {
            section = match.template get<1>().str();
        } else if (section == "libdirs"sv) {
            auto path = std::filesystem::path { ctre::match<R"(^\s*(.*)\s*$)">(line).template get<1>().str(), std::filesystem::path::native_format };
            if (!path.empty())
                result.emplace_back(path);
        }
    }
    return result;
}

int main(int argc, char **argv)
{
    auto program = argparse::ArgumentParser {};
    program
        .add_argument("-V"sv, "--verbose"sv)
        .help("Be verbose"s)
        .default_value(false)
        .implicit_value(true);
    program
        .add_argument("build_dir"sv)
        .required()
        .help("Directory containing conanbuildinfo.txt"s);
    program
        .add_argument("install_dir"sv)
        .required()
        .help("Directory to put libraries"s);

    try {
        program.parse_args(argc, argv);
    } catch (std::runtime_error const &e) {
        fmt::print(stderr, "{}\n", e.what());
        fmt::print(stderr, "{}\n", program.help().str());
        std::exit(EXIT_FAILURE);
    }

    if (program["--verbose"] == true) {
        g_verbose = true;
        fmt::print(stderr, "verbosity enabled\n");
    }

    auto const libdirs = ::libdirs(program.get("build_dir"sv) / std::filesystem::path("conanbuildinfo.txt"));
    auto const install_dir = std::filesystem::path { program.get("install_dir"sv) };

    for (auto const &libdir : libdirs) {
        for (auto const &lib : std::filesystem::recursive_directory_iterator(libdir, std::filesystem::directory_options::none)) {
            if (!ctre::match<R"(.*?\.so(\.\d+)*$)">(lib.path().filename().string())) continue;

            auto const destination = install_dir / lib.path().lexically_relative(libdir);
            if (g_verbose) fmt::print(stderr, "installing {} -> {}\n", lib.path().string(), destination.string());
            std::filesystem::create_directories(destination.parent_path());
            std::filesystem::copy(lib.path(), destination, std::filesystem::copy_options::copy_symlinks);
        }
    }
}