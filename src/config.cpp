#include "Config.hpp"

#include <iostream>
#include <cassert>
#include <algorithm>

static std::string_view rstrip(std::string_view s)
{
    while (!s.empty() && std::isspace(s.back()))
        s.remove_suffix(1);
    return s;
}

static std::string_view lstrip(std::string_view s)
{
    while (!s.empty() && std::isspace(s.front()))
        s.remove_prefix(1);
    return s;
}

static std::string_view strip(std::string_view s) { return lstrip(rstrip(s)); }

static void parse_line(Config &cfg, std::string_view line, std::size_t lnumber)
{
    if (auto pos = line.find('#'); pos != std::string_view::npos)
        line = line.substr(0, pos);

    line = strip(line);

    if (line.empty())
        return;

    if (auto pos = line.find('='); pos == std::string_view::npos)
    {
        char buff[256];
        std::sprintf(buff, "Invalid format at line %z; no equal sign.", lnumber);
        throw std::runtime_error(buff);
    }
    else
    {
        auto key = lstrip(line.substr(0, pos));
        auto val = rstrip(line.substr(pos + 1));
        cfg.set(key, val);
    }
}

Config Config::from_file(FILE *fp)
{
    Config config;
    std::size_t lnumber = 1;
    std::string line;
    char buffer[256];

    while (std::fgets(buffer, sizeof(buffer), fp))
    {
        line += buffer;
        if (line.back() == '\n' || std::feof(fp))
        {
            parse_line(config, line, lnumber);
            line.clear();
            ++lnumber;
        }
    }

    if (std::feof(fp)) /*good*/
    {
    }

    assert(!std::ferror(fp) && "Error occurred while reading from stream.");
    return config;
}

Config Config::from_stream(std::istream &stream)
{
    Config config;
    std::size_t lnumber = 1;
    std::string line;
    while (std::getline(stream, line))
    {
        parse_line(config, line, lnumber);
        ++lnumber;
    }
    assert(!stream.fail() && "Error occurred while reading from stream.");
    return config;
}

Config Config::from_string(std::string_view str)
{
    Config config;
    std::size_t lnumber = 1;
    auto prev = str.begin();

    for (;;)
    {
        auto it = std::find(prev, str.end(), '\n');
        std::string_view line{prev, static_cast<std::size_t>(it - prev)};
        parse_line(config, line, lnumber);
        if (it == str.end())
            break;
        ++prev;
        ++lnumber;
    }
    return config;
}

void Config::set(std::string_view k, std::string_view v)
{
    auto it = std::lower_bound(m_data.begin(), m_data.end(), k, [](auto const &p, auto s) { return p.first < s; });
    if (it == m_data.end())
        m_data.emplace_back(static_cast<std::string>(k), static_cast<std::string>(v));
    else if (it->first == k)
        it->second = static_cast<std::string>(v);
    else
        m_data.insert(it, std::pair<std::string, std::string>{static_cast<std::string>(k), static_cast<std::string>(v)});
}

void Config::set(std::string_view k, std::string &&v)
{
    auto it = std::lower_bound(m_data.begin(), m_data.end(), k, [](auto const &p, auto s) { return p.first < s; });
    if (it == m_data.end())
        m_data.emplace_back(static_cast<std::string>(k), std::move(v));
    else if (it->first == k)
        it->second = std::move(v);
    else
        m_data.insert(it, std::pair<std::string, std::string>{static_cast<std::string>(k), std::move(v)});
}

void Config::set(std::string &&k, std::string &&v)
{
    auto it = std::lower_bound(m_data.begin(), m_data.end(), k, [](auto const &p, auto const &s) { return p.first < s; });
    if (it == m_data.end())
        m_data.emplace_back(std::move(k), std::move(v));
    else if (it->first == k)
        it->second = std::move(v);
    else
        m_data.insert(it, std::pair<std::string, std::string>{std::move(k), std::move(v)});
}

std::optional<std::string_view> Config::get(std::string_view k) const noexcept
{
    auto it = std::lower_bound(m_data.begin(), m_data.end(), k, [](auto const &p, auto const &s) { return p.first < s; });
    if (it == m_data.end() || it->first != k)
        return std::nullopt;
    else
        return static_cast<std::string_view>(it->second);
}

std::string_view Config::get_or(std::string_view k, std::string_view v) const noexcept
{
    auto it = std::lower_bound(m_data.begin(), m_data.end(), k, [](auto const &p, auto const &s) { return p.first < s; });
    if (it == m_data.end() || it->first != k)
        return v;
    else
        return static_cast<std::string_view>(it->second);
}

std::optional<std::string> Config::del(std::string_view k) noexcept
{
    auto it = std::lower_bound(m_data.begin(), m_data.end(), k, [](auto const &p, auto const &s) { return p.first < s; });
    if (it->first != k)
        return std::nullopt;

    std::string v = std::move(it->second);
    m_data.erase(it);
    return std::move(v);
}