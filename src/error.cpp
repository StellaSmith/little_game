#include <utils/error.hpp>

using namespace std::string_view_literals;

[[noreturn]] void utils::show_error(std::string_view body)
{
    throw application_error("ERROR!"sv, body);
}

[[noreturn]] void utils::show_error(std::string_view title, std::string_view body)
{
    throw application_error(title, body);
}