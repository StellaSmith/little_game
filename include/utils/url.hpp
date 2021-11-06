#ifndef UTILS_URL_HPP
#define UTILS_URL_HPP

#include <string>
#include <string_view>

#include <ctre.hpp>

namespace utils {
    struct UrlAuthority {
        std::string_view userinfo;
        std::string_view host;
        std::string_view port;
    };

    struct UrlParseResult {
        std::string_view schema;
        UrlAuthority authority;
        std::string_view path;
        std::string_view query;
        std::string_view fragment;
    };

    static constexpr auto url_pattern = ctll::fixed_string { R"PCRE2(^(?:(?<schema>.+?):)?(?:\/\/(?:(?<userinfo>.+?)@)?(?<host>[^\/\n]+)?(?::(?<port>[^\/\n]+?))?)?(?<path>[^?\n#]+)?(?:\?(?<query>[^\n#]+))?(?:#(?<fragment>.*?))?$)PCRE2" };
    constexpr UrlParseResult parse_url(std::string_view url) noexcept
    {
        auto const match = ctre::match<url_pattern>(url);
        UrlParseResult result;
        result.schema = match.get<1>();
        result.authority.userinfo = match.get<2>();
        result.authority.host = match.get<3>();
        result.authority.port = match.get<4>();
        result.path = match.get<5>();
        result.query = match.get<6>();
        result.fragment = match.get<7>();
        return result;
    }
}

#endif