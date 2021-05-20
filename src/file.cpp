#include <utils/file.hpp>

#include <iterator>
#include <istream>

std::string utils::load_file(std::istream &is)
{
    return { std::istreambuf_iterator<char> { is }, std::istreambuf_iterator<char> {} };
}

std::string utils::load_file(std::FILE *fp)
{
    auto current = std::ftell(fp);
    std::fseek(fp, 0, SEEK_END);
    auto end = std::ftell(fp);
    std::fseek(fp, current, SEEK_SET);

    std::string result(static_cast<std::size_t>(end - current), '\0');
    std::fread(result.data(), result.size(), 1, fp);
    return result;
}