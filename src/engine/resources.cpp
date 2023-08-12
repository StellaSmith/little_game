
#include <boost/outcome/success_failure.hpp>
#include <engine/resources.hpp>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/outcome/try.hpp>

#include <engine/string.hpp>

#include <string_view>
#include <unordered_map>
#include <utility>

thread_local static boost::container::flat_map<std::string_view, resources::BaseResource const *> s_resource_cache;

engine::result<engine::nonnull<resources::BaseResource const>, std::errc> engine::open_resource(std::string_view path) noexcept
{
    path = engine::strip(path, '/');
    if (path.empty())
        return resources::get_root();

    if (auto it = s_resource_cache.find(path); it != s_resource_cache.end())
        return it->second;

    auto pos = path.rfind('/');

    resources::BaseResource const *root;

    if (pos == std::string_view::npos)
        root = resources::get_root();
    else
        root = TRY(open_resource(path.substr(0, pos)));

    if (root != nullptr) {
        if (root->type != resources::ResourceType::DIRECTORY_RESOURCE) {
            root = nullptr;
        } else {
            for (std::size_t i = 0; i < root->size; ++i) {
                if (static_cast<resources::DirectoryResource const *>(root)->entries[i]->path == path) {
                    root = static_cast<resources::DirectoryResource const *>(root)->entries[i];
                    break;
                }
            }
        }
    }

    // make sure the cache doesn't contain null pointers
    if (root == nullptr)
        return std::errc::no_such_file_or_directory;

    return s_resource_cache[std::string_view { root->path, path.size() }] = root;
}

engine::result<std::span<std::byte const>, std::errc> engine::load_resource(std::string_view name) noexcept
{
    boost::container::flat_set<resources::BaseResource const *> visited;
    auto resource = TRY(open_resource(name));
    if (resource == nullptr) // ENOENT
        return std::errc::no_such_file_or_directory;

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