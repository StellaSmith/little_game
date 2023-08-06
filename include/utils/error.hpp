#include <spdlog/spdlog.h>

#include <exception>
#include <string>
#include <string_view>

namespace utils {
    class application_error : std::exception {
    public:
        application_error(std::string_view title, std::string_view body)
            : m_title { title }
            , m_body { body }
            , m_what { m_title + ": " + m_body }
        {
        }

        char const *what() const noexcept
        {
            return m_what.c_str();
        }

        std::string_view title() const noexcept
        {
            return m_title;
        }

        std::string_view body() const noexcept
        {
            return m_body;
        }

    private:
        std::string m_title;
        std::string m_body;
        std::string m_what;
    };

    [[noreturn]] void show_error(std::string_view body);
    [[noreturn]] void show_error(std::string_view title, std::string_view body);

#define THROW_CRITICAL(...)                        \
    do {                                           \
        auto const str = fmt::format(__VA_ARGS__); \
        SPDLOG_CRITICAL(str);                      \
        throw std::runtime_error(str);             \
    } while (0)
} // namespace utils