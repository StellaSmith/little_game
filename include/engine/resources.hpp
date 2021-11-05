#ifndef ENGINE_RESOURCES_HPP
#define ENGINE_RESOURCES_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <resources.cpp>

namespace engine {

    inline resources::BaseResource const *open_resource(char const *path_ /* utf-8 */) noexcept
    {
        auto const *root = resources::get_root();
        assert(root->type == resources::DIRECTORY_RESOURCE); // sanity check

        std::string_view path = path_;

        // path resolution
        for (;;) {
            if (!path.empty() && path[0] == '/')
                path.remove_prefix(1);
            if (path.empty())
                break;

            auto base_index = path.find('/');
            if (base_index == std::string_view::npos)
                base_index = path.size();

            bool found = false;
            std::string_view searching = path.substr(0, base_index);
            for (std::size_t i = 0; i < root->size; ++i) {
                auto const *dir = static_cast<resources::DirectoryResource const *>(root);
                if (dir->entries[i]->basename == searching) {
                    found = true;
                    root = dir->entries[i];
                    path.remove_prefix(base_index);
                    break;
                }
            }
            if (!found)
                return nullptr;
        }

        return root;
    }

}

#endif