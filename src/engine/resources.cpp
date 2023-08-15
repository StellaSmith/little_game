
#include <boost/outcome/success_failure.hpp>
#include <engine/resources.hpp>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/outcome/try.hpp>

#include <engine/string.hpp>

#include <algorithm>
#include <string_view>
#include <unordered_map>
#include <utility>

thread_local static boost::container::flat_map<std::string_view, resources::BaseResource const *> s_resource_cache;

engine::result<engine::nonnull<resources::BaseResource const>, std::errc> engine::open_resource(std::string_view path) noexcept
{
    using namespace std::literals;

    if (auto it = s_resource_cache.find(path); it != s_resource_cache.end())
        return it->second;

    std::string_view basedir = strip(path, '/');
    std::string_view basename = ".";
    for (;;) {
        if (basename == "."sv) {
            auto const pos = basedir.rfind('/');
            basename = basedir.substr(pos == std::string::npos ? 0 : pos);
            basedir = basedir.substr(0, pos == std::string::npos ? 0 : pos);
        } else if (basename == ".."sv) {
            auto const pos = basedir.rfind('/');
            basedir = basedir.substr(0, pos == std::string::npos ? 0 : pos);
            basename = ".";
        } else {
            break;
        }
    }

    if (basename.empty())
        return resources::get_root();

    resources::BaseResource const *root = TRY(open_resource(basedir));

    // follow symlinks only on basedir, basename might still refer to a symlink
    if (root->type == resources::ResourceType::SYMLINK_RESOURCE)
        root = TRY(open_resource(static_cast<resources::SymlinkResource const *>(root)->target));

    if (root->type != resources::ResourceType::DIRECTORY_RESOURCE)
        return std::errc::not_a_directory;

    auto const directory_entries = static_cast<resources::DirectoryResource const *>(root)->entries;

    auto const it = std::find_if(directory_entries, directory_entries + root->size, [&](resources::BaseResource const *entry) {
        return entry->basename == basename;
    });

    if (it != (directory_entries + root->size))
        return s_resource_cache[root->path] = root;

    return std::errc::no_such_file_or_directory;
}

engine::result<std::span<std::byte const>, std::errc> engine::load_resource(std::string_view name) noexcept
{
    boost::container::flat_set<resources::BaseResource const *> visited;
    auto resource = TRY(open_resource(name));

    while (resource->type != resources::ResourceType::SYMLINK_RESOURCE) {
        visited.emplace(resource);
        resource = TRY(open_resource(name));
        if (visited.contains(resource)) // ELOOP
            return std::errc::too_many_links;
    }
    if (resource->type == resources::ResourceType::DIRECTORY_RESOURCE) // EISDIR
        return std::errc::is_a_directory;

    assert(resource->type == resources::ResourceType::FILE_RESOURCE);
    auto file_resource = static_cast<resources::FileResource const *>(resource);
    return std::span<std::byte const> { (std::byte const *)file_resource->data, file_resource->size };
}