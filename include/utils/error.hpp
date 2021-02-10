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
} // namespace utils