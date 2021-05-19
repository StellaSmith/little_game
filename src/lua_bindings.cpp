#include "engine/game.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

#include <exception>
#include <string>
#include <system_error>

using json = nlohmann::json;
using namespace std::literals;

extern json g_config_engine;
extern bool g_verbose;

#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#define HAS_CXXABI_H
#endif

static std::string unmangled_name(std::type_info const &ti)
{
#ifdef HAS_CXXABI_H
    std::size_t length;
    int status;
    char *n = abi::__cxa_demangle(ti.name(), nullptr, &length, &status);
    std::string result { n, length };
    free(n);
    return result;
#else
    return ti.name();
#endif
}

static int exception_handler(lua_State *L, sol::optional<std::exception const &> possible_exception, sol::string_view description) noexcept
{
    if (possible_exception) {
        std::exception const &exception = *possible_exception;
        if (typeid(exception) == typeid(std::system_error)) {
            auto const &system_error = dynamic_cast<std::system_error const &>(exception);
            spdlog::error("[sol2] Unhandled C++ system_error; category={} message=\"{}\" desc=\"{}\"",
                system_error.code().category().name(),
                system_error.code().message(),
                description);
        } else {
            spdlog::error("[sol2] Unhandled C++ exception; typeid={} what=\"{}\" desc=\"{}\"",
                unmangled_name(typeid(exception)),
                exception.what(),
                description);
        }
    } else {
        spdlog::error("[sol2] Unhandled C++ exception; desc=\"{}\"", description);
    }
    return sol::stack::push(L, description);
}

namespace {
    struct Printer {
        std::uint32_t max_lines;
        std::deque<std::string> &console_text;
        void operator()(sol::variadic_args args, sol::this_state L)
        {
            // this looks ugly
            std::string line;
            for (auto arg : args) {
                bool got_string = false;
                if (auto mt = arg.get<sol::optional<sol::metatable>>(); mt.has_value()) {
                    if (auto to_string = mt->raw_get<sol::optional<sol::stack_proxy>>("__tostring"sv); to_string.has_value()) {
                        if (to_string->get_type() == sol::type::function) {
                            if (auto results = to_string->as<sol::function>().call(arg); results.valid()) {
                                if (auto s = results.get<sol::optional<std::string_view>>(); s.has_value()) {
                                    line += *s;
                                    got_string = true;
                                }
                            }
                        } else if (to_string->get_type() == sol::type::string) {
                            line += to_string->as<std::string_view>();
                            got_string = true;
                        }
                    }
                }
                if (!got_string) {
                    if (arg.get_type() == sol::type::string)
                        line += arg.as<std::string_view>();
                    else if (arg.get_type() == sol::type::boolean)
                        line += arg.as<bool>() ? "true"sv : "false"sv;
                    else if (arg.get_type() == sol::type::number)
                        fmt::format_to(std::back_inserter(line), "{}"sv, arg.as<lua_Number>());
                    else {
                        std::string const &type_name = sol::type_name(L, arg.get_type());
                        auto address = reinterpret_cast<std::uintptr_t>(arg.as<sol::reference>().pointer());
                        fmt::format_to(std::back_inserter(line), "<{0} at 0x{1:0{2}X}>"sv, type_name, address, sizeof(address) * 2);
                    }
                }
                line += '\t';
            }

            line.pop_back();

            console_text.emplace_back(std::move(line));
            while (console_text.size() > max_lines)
                console_text.pop_front();
        }
    };
} // namespace

void engine::Game::setup_lua()
{
    m_lua = sol::state {};
    m_lua.set_exception_handler(&exception_handler);
    m_lua.globals().raw_set("_VERBOSE"sv, g_verbose);

    std::uint32_t max_lines = 5000;

    try {
        g_config_engine.at("/Terminal/max_lines"_json_pointer).get_to(max_lines);
        if (max_lines > std::numeric_limits<lua_Integer>::max())
            throw std::range_error(fmt::format("/Terminal/max_lines cant be bigger than {}", std::numeric_limits<lua_Integer>::max()));
    } catch (std::exception &e) {
        if (g_verbose)
            spdlog::warn("Can't obtain /Terminal/max_lines, using the default of {}\n\t{}\n", max_lines, e.what());
    }

    m_lua.open_libraries(sol::lib::base);
    // m_lua.open_libraries(sol::lib::package); disabled
    m_lua.open_libraries(sol::lib::coroutine);
    m_lua.open_libraries(sol::lib::string);
    // m_lua.open_libraries(sol::lib::os); disabled
    m_lua.open_libraries(sol::lib::math);
    m_lua.open_libraries(sol::lib::table);
    // m_lua.open_libraries(sol::lib::debug); disabled
    m_lua.open_libraries(sol::lib::bit32);
    // m_lua.open_libraries(sol::lib::io); disabled
    // m_lua.open_libraries(sol::lib:ffi); disabled (might enable some functionality later)
    // m_lua.open_libraries(sol::lib::jit); disabled (might enable some functionality later)
    m_lua.open_libraries(sol::lib::utf8);

    // disable string.dump
    m_lua.globals().raw_get<sol::table>("string"sv).raw_set("dump"sv, sol::nil);

    // disable these globals
    for (auto f : { "collectgarbage"sv, "dofile"sv, "load"sv, "loadfile"sv })
        m_lua.globals().raw_set(f, sol::nil);

    // override print
    m_lua.globals().set_function("print"sv, Printer { max_lines, this->m_console_text });

    sol::environment sv_env(m_lua, sol::create, m_lua.globals());
    sol::environment cl_env(m_lua, sol::create, m_lua.globals());

    if (auto result = m_lua.load_file("lua/sv_init.lua"s, sol::load_mode::text); !result.valid()) {
        sol::error error = result;
        spdlog::error("Failed to load server script: {} {}", sol::to_string(result.status()), error.what());
    } else {
        sol::protected_function func = result;
        sv_env.set_on(func);
        if (auto function_result = func.call(); !function_result.valid()) {
            try {
                sol::error e = function_result.get<sol::error>();
                spdlog::error("Failed to run server script: {} {}", sol::to_string(function_result.status()), e.what());
            } catch (sol::error &e) {
                spdlog::error("Failed to run server script: {} {}", sol::to_string(function_result.status()), e.what());
            }
        }
    }

    if (auto result = m_lua.load_file("lua/cl_init.lua"s, sol::load_mode::text); !result.valid()) {
        sol::error error = result;
        spdlog::error("Can't load client script: {}", error.what());
    } else {
        sol::protected_function func = result;
        sol::stack::push(m_lua, &sol::default_traceback_error_handler);
        cl_env.set_on(func);
        if (auto function_result = func.call(); !function_result.valid()) {
            try {
                sol::error e = function_result.get<sol::error>();
                spdlog::error("Failed to run client script: {} {}", sol::to_string(function_result.status()), e.what());
            } catch (sol::error &e) {
                spdlog::error("Failed to run client script: {} {}", sol::to_string(function_result.status()), e.what());
            }
        }
    }
}