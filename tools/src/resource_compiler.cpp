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

static bool g_verbose = false;

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

static std::vector<std::byte> minify_json(boost::filesystem::path const &filename, std::vector<std::byte> data);

std::unique_ptr<FileResource> add_file(boost::filesystem::path const &path)
{
    auto result = std::make_unique<FileResource>();
    result->path = path;
    result->data = load_file(path);

    using namespace std::literals;
    if (path.extension().string() == ".json"sv || path.extension().string() == ".jsonc"sv) {
        result->data = minify_json(path, std::move(result->data));
        if (g_verbose) fmt::print(stderr, "including file {} as minified json\n", std::quoted(path.string()));
    } else {
        if (g_verbose) fmt::print(stderr, "including file {} as is\n", std::quoted(path.string()));
    }
    return result;
}

std::vector<std::unique_ptr<BaseResource>> add_directory(boost::filesystem::path const &path)
{
    std::vector<std::unique_ptr<BaseResource>> result;
    auto current = std::make_unique<DirectoryResource>();
    current->path = path;

    if (g_verbose) fmt::print(stderr, "scanning directory {}\n", std::quoted(path.string()));
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
    if (g_verbose) fmt::print(stderr, "including directory {} as is\n", std::quoted(path.string()));
    return result;
}

#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>
#include <rapidjson/stream.h>
#include <rapidjson/writer.h>

struct IOMemoryStream {
    using Ch = char;

    IOMemoryStream(char *src, size_t size)
        : src_(src)
        , begin_(src)
        , end_(src + size)
        , size_(size)
    {
    }

    char Peek() const
    {
        return RAPIDJSON_UNLIKELY(src_ == end_) ? '\0' : *src_;
    }

    char Take()
    {
        return RAPIDJSON_UNLIKELY(src_ == end_) ? '\0' : *src_++;
    }

    size_t Tell() const
    {
        return static_cast<size_t>(src_ - begin_);
    }

    char *PutBegin()
    {
        RAPIDJSON_ASSERT(false);
        return 0;
    }

    void Put(char c)
    {
        RAPIDJSON_ASSERT(src_ != end_);
        *src_++ = c;
    }

    void Flush()
    {
    }

    size_t PutEnd(char *)
    {
        RAPIDJSON_ASSERT(false);
        return 0;
    }

    // For encoding detection only.
    const char *Peek4() const
    {
        return Tell() + 4 <= size_ ? src_ : 0;
    }

    char *src_; //!< Current read position.
    char *begin_; //!< Original head of the string.
    char *end_; //!< End of stream.
    size_t size_; //!< Size of the stream.
};

template <typename Container>
auto back_inserter_stream(Container &container)
{
    struct BackInserterStream {
        using Ch = std::conditional_t<std::is_same_v<typename Container::value_type, std::byte>, char, typename Container::value_type>;

        void Put(Ch c)
        {
            auto const prev = container.capacity();
            container.push_back(static_cast<typename Container::value_type>(c));
            if (container.capacity() != prev)
                fmt::print(stderr, "allocation\n");
        }

        void Flush() const noexcept { }

        Container &container;
    };

    return BackInserterStream { container };
}

static std::vector<std::byte> minify_json(boost::filesystem::path const &path, std::vector<std::byte> data)
{
    rapidjson::MemoryStream is(reinterpret_cast<char *>(data.data()), data.size());
    rapidjson::Reader reader;

    std::vector<std::byte> result;
    result.reserve(data.size() / 4u * 3u);
    auto os = back_inserter_stream(result);

    rapidjson::Writer<decltype(os)> writer(os);

    if (!reader.Parse<
            rapidjson::kParseCommentsFlag
            | rapidjson::kParseFullPrecisionFlag
            | rapidjson::kParseNanAndInfFlag
            | rapidjson::kParseTrailingCommasFlag>(is, writer)) {
        fmt::print(stderr, "Error parsing json file {} at offset {} {}\n",
            std::quoted(path.string()),
            reader.GetErrorOffset(), rapidjson::GetParseError_En(reader.GetParseErrorCode()));
        fmt::print(stderr, "Skiping minification\n");

        return data;
    }

    return result;
}

int main(int argc, char **argv)
{
    using namespace std::literals;

    auto program = argparse::ArgumentParser {};
    program
        .add_argument("-V", "--verbose")
        .help("Be verbose")
        .default_value(false)
        .implicit_value(true);
    program
        .add_argument("resources_dir")
        .help("Directory containing the resources to compile.");
    program
        .add_argument("-o", "--output")
        .default_value("-"s)
        .help("File to put the output");
    try {
        program.parse_args(argc, argv);
    } catch (std::runtime_error const &e) {
        fmt::print(stderr, "{}\n", e.what());
        fmt::print(stderr, "{}\n", program);
        std::exit(EXIT_FAILURE);
    }

    if (program["--verbose"] == true) {
        g_verbose = true;
        fmt::print(stderr, "verbosity enabled\n");
    }

    std::FILE *output = stdout;
    if (auto output_path = program.get<std::string>("--output"); output_path != "-") {
        output = std::fopen(output_path.c_str(), "wt");
        if (!output) {
            fmt::print(stderr, "failed to open {} for writting: ({}) {}\n", std::quoted(output_path), errno, std::strerror(errno));
            std::exit(EXIT_FAILURE);
        }
        std::setvbuf(output, nullptr, _IOFBF, 1024 * 8);
    }

    if (program.present("resources_dir"))
        boost::filesystem::current_path(program.template get<std::string>("resources_dir"));

    fmt::print(output, "namespace resources {{\n\n"sv);
    fmt::print(output, "enum class ResourceType {{\n    DIRECTORY_RESOURCE,\n    FILE_RESOURCE\n}};\n\n"sv);
    fmt::print(output, "struct BaseResource {{\n    ResourceType type;\n    char const *path;\n    char const *basename;\n    unsigned long long size;\n}};\n\n"sv);
    fmt::print(output, "struct DirectoryResource : BaseResource {{\n    BaseResource const *const *entries;\n}};\n\n"sv);
    fmt::print(output, "struct FileResource : BaseResource {{\n    unsigned char const *data;\n}};\n\n"sv);
    fmt::print(output, "BaseResource const *get_root() noexcept;\n\n"sv);
    fmt::print(output, "}}\n\n"sv);
    fmt::print(output, "#if defined(COMPILE_RESOURCES)\n\n"sv);

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

        fmt::print(output, "static char const entry_{}_path[] = {};\n", i, ascii_repr(path));
        if (auto p_dir = dynamic_cast<DirectoryResource *>(entry.get())) {
            if (!p_dir->entries.empty()) {
                fmt::print(output, "static resources::BaseResource const *const entry_{}_data[] = {{\n", i);
                std::sort(p_dir->entries.begin(), p_dir->entries.end());
                for (auto const &e : p_dir->entries) {
                    fmt::print(output, "    &entry_{},\n", indices[e.native()]);
                }
                fmt::print(output, "}};\n");

                fmt::print(output, "static resources::DirectoryResource const entry_{} = {{\n", i);
                fmt::print(output, "    resources::ResourceType::DIRECTORY_RESOURCE,\n");
                fmt::print(output, "    &entry_{}_path[0],\n", i);
                fmt::print(output, "    &entry_{}_path[{}],\n", i, name_index);

                if (!p_dir->entries.empty()) {
                    fmt::print(output, "    sizeof(entry_{0}_data) / sizeof(entry_{0}_data[0]),\n", i);
                    fmt::print(output, "    &entry_{}_data[0],\n", i);
                } else {
                    fmt::print(output, "    0,\n");
                    fmt::print(output, "    nullptr,\n");
                }

                fmt::print(output, "}};\n");
            }
        } else if (auto *p_file = reinterpret_cast<FileResource *>(entry.get())) {
            if (!p_file->data.empty()) {
                fmt::print(output, "static unsigned char const entry_{}_data[] = {{\n", i);
                for (std::size_t j = 0; j < p_file->data.size(); j += 16) {
                    fmt::print(output, "    {},\n",
                        fmt::join(
                            reinterpret_cast<unsigned char const *>(p_file->data.data() + j),
                            reinterpret_cast<unsigned char const *>(p_file->data.data() + std::min(j + 16u, p_file->data.size())),
                            ", "));
                }
                fmt::print(output, "}};\n");
            }

            fmt::print(output, "static resources::FileResource const entry_{} = {{\n", i);
            fmt::print(output, "    resources::ResourceType::FILE_RESOURCE,\n");
            fmt::print(output, "    &entry_{}_path[0],\n", i);
            fmt::print(output, "    &entry_{}_path[{}],\n", i, name_index);

            if (!p_file->data.empty()) {
                fmt::print(output, "    sizeof(entry_{}_data),\n", i);
                fmt::print(output, "    &entry_{}_data[0],\n", i);
            } else {
                fmt::print(output, "    0,\n");
                fmt::print(output, "    nullptr,\n");
            }

            fmt::print(output, "}};\n");
        }
        fmt::print(output, "\n");
        ++i;
    }

    fmt::print(output, "resources::BaseResource const *resources::get_root() noexcept\n");
    fmt::print(output, "{{\n");
    fmt::print(output, "    return &entry_{};\n", i - 1);
    fmt::print(output, "}}\n\n");
    fmt::print(output, "#endif\n");
}