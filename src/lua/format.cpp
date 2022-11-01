#include <boost/container/flat_set.hpp>
#include <fmt/format.h>
#include <sol/sol.hpp>

using namespace std::literals;

namespace lua::impl {
    std::string repr(sol::object obj, boost::container::flat_set<void const *> &visited);
}

static std::string repr_string(std::string_view str)
{
    std::string result;
    result.reserve(str.size() + 2);
    result += '"';
    for (auto const c : str) {
        if (c == '\0')
            result += "\\0"sv;
        else if (c == '"')
            result += "\""sv;
        else if (c == '\\')
            result += "\\\\"sv;
        else if (c == '\a')
            result += "\\a"sv;
        else if (c == '\b')
            result += "\\b"sv;
        else if (c == '\f')
            result += "\\f"sv;
        else if (c == '\n')
            result += "\\n"sv;
        else if (c == '\r')
            result += "\\r"sv;
        else if (c == '\t')
            result += "\\t"sv;
        else if (c == '\v')
            result += "\\v"sv;
        else if (' ' <= c && c <= '~')
            result += c;
        else
            fmt::format_to(std::back_inserter(result), "\\x{:02}", static_cast<unsigned char>(c));
    }
    result += '"';
    return result;
}

static std::string repr_table(sol::table tbl, boost::container::flat_set<void const *> &visited)
{
    std::string result;
    result += '{';
    tbl.for_each([&](sol::object k, sol::object v) {
        result += '[';
        result += lua::impl::repr(std::move(k), visited);
        result += "] = "sv;
        result += lua::impl::repr(std::move(v), visited);
        result += ", "sv;
    });
    if (result.back() == ' ')
        result.back() = '}';
    else
        result += '}';

    return result;
}

namespace lua::impl {

    std::string repr(sol::object obj, boost::container::flat_set<void const *> &visited)
    {
        {
            std::vector<void const *> constdbg_visited(visited.begin(), visited.end());
            if (visited.contains(obj.pointer()))
                return "...";
        }

        obj.push();
        if (lua_getmetatable(obj.lua_state(), -1)) {
            auto metatable = sol::metatable { obj.lua_state(), -1 };
            lua_pop(obj.lua_state(), 2);
            if (metatable["__repr"].valid()) {
                visited.emplace(obj.pointer());
                return metatable["__repr"].call<std::string>(obj);
            }
        } else {
            lua_pop(obj.lua_state(), 1);
        }

        switch (obj.get_type()) {
        case sol::type::nil:
            return "nil";
        case sol::type::none:
            return "none";
        case sol::type::boolean:
            if (obj.as<bool>())
                return "true";
            else
                return "false";
        case sol::type::thread:
        case sol::type::function:
        case sol::type::lightuserdata:
        case sol::type::userdata:
            return fmt::format("<{0} at 0x{1:0{2}X}>",
                sol::type_name(obj.lua_state(), obj.get_type()),
                reinterpret_cast<std::uintptr_t>(obj.pointer()),
                sizeof(void *) * 2);
        case sol::type::string:
            return repr_string(obj.as<std::string>());
        case sol::type::number:
            return fmt::format("{}", obj.as<lua_Number>());
        case sol::type::table:
            visited.emplace(obj.pointer());
            return repr_table(obj.as<sol::table>(), visited);
        case sol::type::poly:
            throw std::runtime_error("what's a poly");
        }
        return "???"s;
    }

    std::string repr(sol::object obj)
    {
        boost::container::flat_set<void const *> visited;
        return repr(std::move(obj), visited);
    }

}
