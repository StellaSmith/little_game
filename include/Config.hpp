#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

class Config {
public:
    static Config from_file(FILE *fp);
    static Config from_string(std::string_view str);
    static Config from_stream(std::istream &stream);

    std::optional<std::string_view> get(std::string_view k) const noexcept;
    std::string_view get_or(std::string_view k, std::string_view v) const noexcept;

    void set(std::string_view k, std::string_view v);
    void set(std::string_view k, std::string &&v);
    void set(std::string &&k, std::string &&v);

    std::optional<std::string> del(std::string_view k) noexcept;

    inline auto crbegin() const noexcept
    {
        return m_data.crbegin();
    }

    inline auto crend() const noexcept
    {
        return m_data.crend();
    }

    inline auto cbegin() const noexcept
    {
        return m_data.cbegin();
    }

    inline auto cend() const noexcept
    {
        return m_data.cend();
    }

private:
    std::vector<std::pair<std::string, std::string>> m_data;
};

#endif