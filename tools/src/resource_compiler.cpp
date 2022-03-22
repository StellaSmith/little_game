#include <filesystem> // just to convert paths to utf8
#include <memory>
#include <string>
#include <vector>

#include <argparse/argparse.hpp>
#include <boost/container/map.hpp>
#include <boost/filesystem.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h> // formatting boost::filesystem::path
#include <fmt/ranges.h>

struct BaseResource {
    boost::filesystem::path path;
    virtual ~BaseResource() = default;
};

struct DirectoryResource : BaseResource {
    std::vector<boost::filesystem::path> entries;
};

struct FileResource : BaseResource {
    std::vector<std::byte> data;
};

std::FILE *open_file(boost::filesystem::path const &path, char const *mode) noexcept
{
#ifdef _WIN32
    wchar_t buf[32] {};
    for (int i = 0; i <= std::size(buf); ++i) {
        if (i == std::size(buf))
            throw std::length_error("mode is too long");
        buf[i] = mode[i];
    }
    return _wfopen(path.c_str(), buf);
#else
    return std::fopen(path.c_str(), mode);
#endif
}

std::string ascii_repr(std::u8string_view str)
{
    std::string result;
    result.reserve(str.size());
    result.push_back('"');
    for (auto const c : str) {
        if (c == '\\' || c == '"') {
            result.push_back('\\');
            result.push_back(c);
        } else if (' ' <= c && c <= '~') {
            result.push_back(static_cast<char>(c));
        } else {
            char buf[8] {};
            int len = std::snprintf(buf, 8, "\\x%02x", static_cast<unsigned char>(c));
            result.insert(result.end(), buf, buf + len);
        }
    }
    result.push_back('"');

    return result;
}

std::vector<std::byte> load_file(boost::filesystem::path const &path)
{
    std::unique_ptr<std::FILE, void (*)(std::FILE *)> fp(open_file(path, "rb"), [](std::FILE *fp) {
        std::fclose(fp);
    });
    if (!fp)
        throw boost::system::system_error(errno, boost::system::system_category(), "std::fopen");

    std::setvbuf(fp.get(), nullptr, _IOFBF, 1024 * 8);
    auto const size = boost::filesystem::file_size(path);

    if (size >= SIZE_MAX)
        throw std::length_error("can't possibly fit the file in memory");

    std::vector<std::byte> result(size);
    auto const read = std::fread(result.data(), 1, result.size(), fp.get());

    if (read == size)
        return result;
    else
        throw boost::system::system_error(errno, boost::system::system_category(), "std::fread");
}

std::unique_ptr<FileResource> add_file(boost::filesystem::path const &path)
{
    auto result = std::make_unique<FileResource>();
    result->path = path;
    result->data = load_file(path);
    fmt::print(stderr, "including file {} as is\n", std::quoted(path.string()));
    return result;
}

std::vector<std::unique_ptr<BaseResource>> add_directory(boost::filesystem::path const &path)
{
    std::vector<std::unique_ptr<BaseResource>> result;
    auto current = std::make_unique<DirectoryResource>();
    current->path = path;

    fmt::print(stderr, "scanning directory {}\n", std::quoted(path.string()));
    for (auto const &entry : boost::filesystem::directory_iterator(path)) {

        if (entry.symlink_status().type() == boost::filesystem::file_type::directory_file) {
            current->entries.push_back(entry.path());
            auto tmp = add_directory(entry.path());
            std::move(tmp.begin(), tmp.end(), std::back_inserter(result));
        } else if (entry.symlink_status().type() == boost::filesystem::file_type::regular_file) {
            current->entries.push_back(entry.path());
            result.push_back(add_file(entry.path()));
        }
    }
    result.push_back(std::move(current));
    fmt::print(stderr, "including directory {} as is\n", std::quoted(path.string()));
    return result;
}

int main(int argc, char **argv)
{
    auto program = argparse::ArgumentParser {};
    program
        .add_argument("resources_dir")
        .help("Directory containing the resources to compile.");
    try {
        program.parse_args(argc, argv);
    } catch (std::runtime_error const &e) {
        std::cerr << e.what() << '\n';
        std::cerr << program << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (program.present("resources_dir"))
        boost::filesystem::current_path(program.template get<std::string>("resources_dir"));

    using namespace std::literals;
    fmt::print("namespace resources {{\n\n"sv);
    fmt::print("enum class ResourceType {{\n    DIRECTORY_RESOURCE,\n    FILE_RESOURCE\n}};\n\n"sv);
    fmt::print("struct BaseResource {{\n    ResourceType type;\n    char const *path;\n    char const *basename;\n    unsigned long long size;\n}};\n\n"sv);
    fmt::print("struct DirectoryResource : BaseResource {{\n    BaseResource const *const *entries;\n}};\n\n"sv);
    fmt::print("struct FileResource : BaseResource {{\n    unsigned char const *data;\n}};\n\n"sv);
    fmt::print("BaseResource const *get_root() noexcept;\n\n"sv);
    fmt::print("}}\n\n"sv);
    fmt::print("#if defined(COMPILE_RESOURCES)\n\n"sv);

    auto indices = boost::container::map<std::basic_string_view<boost::filesystem::path::value_type>, std::size_t> {};

    auto const entries = add_directory(boost::filesystem::current_path());
    std::size_t i = 0;
    for (const auto &entry : entries) {
        indices[entry->path.native()] = i;
        auto const path = [&]() {
            auto path = std::filesystem::path(entry->path.lexically_relative(boost::filesystem::current_path()).native(), std::filesystem::path::format::native_format).generic_u8string();
            if (path == u8"."sv)
                path = u8"/"s;
            return path;
        }();
        auto const name = [&]() {
            auto name = std::filesystem::path(entry->path.lexically_relative(boost::filesystem::current_path()).filename().native(), std::filesystem::path::format::native_format).u8string();
            if (name == u8"."sv) name = u8"/"sv;
            return name;
        }();
        auto const name_index = path.rfind(name);

        fmt::print("static char const entry_{}_path[] = {};\n", i, ascii_repr(path));
        if (auto p_dir = dynamic_cast<DirectoryResource *>(entry.get())) {
            if (!p_dir->entries.empty()) {
                fmt::print("static resources::BaseResource const *const entry_{}_data[] = {{\n", i);
                std::sort(p_dir->entries.begin(), p_dir->entries.end());
                for (auto const &e : p_dir->entries) {
                    fmt::print("    &entry_{},\n", indices[e.native()]);
                }
                fmt::print("}};\n");

                fmt::print("static resources::DirectoryResource const entry_{} = {{\n", i);
                fmt::print("    resources::ResourceType::DIRECTORY_RESOURCE,\n");
                fmt::print("    &entry_{}_path[0],\n", i);
                fmt::print("    &entry_{}_path[{}],\n", i, name_index);

                if (!p_dir->entries.empty()) {
                    fmt::print("    sizeof(entry_{0}_data) / sizeof(entry_{0}_data[0]),\n", i);
                    fmt::print("    &entry_{}_data[0],\n", i);
                } else {
                    fmt::print("    0,\n");
                    fmt::print("    nullptr,\n");
                }

                fmt::print("}};\n");
            }
        } else if (auto *p_file = reinterpret_cast<FileResource *>(entry.get())) {
            if (!p_file->data.empty()) {
                fmt::print("static unsigned char const entry_{}_data[] = {{\n", i);
                for (std::size_t j = 0; j < p_file->data.size(); j += 16) {
                    fmt::print("    {},\n",
                        fmt::join(
                            reinterpret_cast<unsigned char const *>(p_file->data.data() + j),
                            reinterpret_cast<unsigned char const *>(p_file->data.data() + std::min(j + 16u, p_file->data.size())),
                            ", "));
                }
                fmt::print("}};\n");
            }

            fmt::print("static resources::FileResource const entry_{} = {{\n", i);
            fmt::print("    resources::ResourceType::FILE_RESOURCE,\n");
            fmt::print("    &entry_{}_path[0],\n", i);
            fmt::print("    &entry_{}_path[{}],\n", i, name_index);

            if (!p_file->data.empty()) {
                fmt::print("    sizeof(entry_{}_data),\n", i);
                fmt::print("    &entry_{}_data[0],\n", i);
            } else {
                fmt::print("    0,\n");
                fmt::print("    nullptr,\n");
            }

            fmt::print("}};\n");
        }
        fmt::print("\n");
        ++i;
    }

    fmt::print("resources::BaseResource const *resources::get_root() noexcept\n");
    fmt::print("{{\n");
    fmt::print("    return &entry_{};\n", i - 1);
    fmt::print("}}\n\n");
    fmt::print("#endif\n");
}