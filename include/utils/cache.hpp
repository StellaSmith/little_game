#include <utils/FileHandle.hpp>

#include <filesystem>
#include <span>
#include <string_view>

namespace utils {
    std::filesystem::path const &cache_directory();

    /**
     * @brief Create a cache file
     *
     * @param name Cache name entry
     * @return std::FILE* The new file open in write binary mode, or null on failure
     */

    FileHandle create_cache_file(std::string_view name);

    /**
     * @brief Get the cache file
     *
     * @param name Cache name entry
     * @param ref_files References for cache time
     * @return std::FILE* The file open in read binary mode, or null on failure
     */
    FileHandle get_cache_file(std::string_view name, std::span<std::filesystem::path const> ref_files = {});
} // namespace utils
