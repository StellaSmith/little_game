#include <engine/Config.hpp>
#include <engine/Game.hpp>

#include <fmt/format.h>
#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

#include <exception>
#include <string>
#include <system_error>

using namespace std::literals;

extern bool g_verbose; // TODO: Move to it's own header

#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#define HAS_CXXABI_H
#endif

static std::string unmangled_name(std::type_info const &ti) // TODO: Move to it's own header
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
        boost::circular_buffer<std::string> &console_text;
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
                    else if (arg.get_type() == sol::type::nil)
                        line += "nil"sv;
                    else {
                        std::string const &type_name = sol::type_name(L, arg.get_type());
                        auto address = reinterpret_cast<std::uintptr_t>(arg.as<sol::reference>().pointer());
                        fmt::format_to(std::back_inserter(line), "<{0} at 0x{1:0{2}X}>"sv, type_name, address, sizeof(address) * 2);
                    }
                }
                line += '\t';
            }

            line.pop_back();
            spdlog::info("[{}lua{}] {}", "\033[36m" /* cyan */, "\033[m" /* reset */, line);

            console_text.push_back(std::move(line));
        }
    };
} // namespace

namespace lua::impl {
    std::string repr(sol::object);
}

void engine::Game::setup_lua()
{
    m_lua = sol::state {};
    m_lua.set_exception_handler(&exception_handler);
    m_lua.globals().raw_set("_VERBOSE"sv, g_verbose);

    m_lua.open_libraries(sol::lib::base);
    // m_lua.open_libraries(sol::lib::package); disabled
    m_lua.open_libraries(sol::lib::coroutine);
    m_lua.open_libraries(sol::lib::string);
    // m_lua.open_libraries(sol::lib::os); disabled
    m_lua.open_libraries(sol::lib::math);
    m_lua.open_libraries(sol::lib::table);
    // m_lua.open_libraries(sol::lib::debug); disabled (might enable some functionality later)
    m_lua.open_libraries(sol::lib::bit32);
    // m_lua.open_libraries(sol::lib::io); disabled
    // m_lua.open_libraries(sol::lib:ffi); disabled (might enable some functionality later)
    // m_lua.open_libraries(sol::lib::jit); disabled (might enable some functionality later)
    m_lua.open_libraries(sol::lib::utf8);

    {
        auto vgame = m_lua.create_table();
        vgame.set_function("repr"sv, &lua::impl::repr);
        m_lua.globals().raw_set("vgame"sv, std::move(vgame));
    }

    // disable string.dump
    m_lua.globals()
        .raw_get<sol::table>("string"sv)
        .raw_set("dump"sv, sol::nil);

    // disable these globals
    for (auto f : { "collectgarbage"sv, "dofile"sv, "load"sv, "loadfile"sv })
        m_lua.globals().raw_set(f, sol::nil);

    // override print
    m_lua.globals().set_function("print"sv, Printer { this->m_console_text });

    sol::environment sv_env(m_lua, sol::create, m_lua.globals());
    m_lua.safe_script_file("lua/sv_init.lua"s, sv_env, [](lua_State *, sol::protected_function_result pfr) {
        spdlog::error("Failed to run server script: {} {}", sol::to_string(pfr.status()), pfr.get<sol::error>().what());
        return pfr;
    });

    sol::environment cl_env(m_lua, sol::create, m_lua.globals());
    m_lua.safe_script_file("lua/cl_init.lua"s, cl_env, [](lua_State *, sol::protected_function_result pfr) {
        spdlog::error("Failed to run client script: {} {}", sol::to_string(pfr.status()), pfr.get<sol::error>().what());
        return pfr;
    });
}