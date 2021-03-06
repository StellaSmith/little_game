#include <cstdio>
#include <string_view>
#include <vector>

namespace utils {
    /**
     * @brief Create a cache file
     * 
     * @param name Cache name entry
     * @return std::FILE* The new file open in write binary mode, or null on failure
     */
    std::FILE *create_cache_file(std::string_view name);

    /**
     * @brief Get the cache file
     * 
     * @param name Cache name entry
     * @param ref_files References for cache time
     * @return std::FILE* The file open in read binary mode, or null on failure
     */
    std::FILE *get_cache_file(std::string_view name, std::vector<std::string_view> ref_files = {});
} // namespace utils
