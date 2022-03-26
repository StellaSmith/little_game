#include <engine/Stream.hpp>
#include <utils/strings.hpp>

#include <boost/container/flat_map.hpp>

#include <string_view>
#include <unordered_map>

thread_local static boost::container::flat_map<std::string_view, resources::BaseResource const *> s_resource_cache;

resources::BaseResource const *engine::open_resource(std::string_view path) noexcept
{
    path = utils::strip(path, '/');
    if (path.empty())
        return resources::get_root();
    if (auto it = s_resource_cache.find(path); it != s_resource_cache.end())
        return it->second;

    auto pos = path.rfind('/');

    resources::BaseResource const *root;

    if (pos == std::string_view::npos)
        root = resources::get_root();
    else
        root = open_resource(path.substr(0, pos));

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

    return s_resource_cache[std::string_view { root->path, path.size() }] = root;
}
