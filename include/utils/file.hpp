#ifndef UTILS_FILE_HPP
#define UTILS_FILE_HPP

#include <cstdio>
#include <iosfwd>
#include <string>

namespace utils {
    std::string load_file(std::istream &is);
    std::string load_file(std::FILE *fp);
} // namespace utils

#endif