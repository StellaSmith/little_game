#include <engine/Config.hpp>
#include <engine/Game.hpp>

#include <fmt/format.h>
#include <sol/sol.hpp>
#include <spdlog/spdlog.h>

#include <exception>
#include <string>
#include <system_error>

using namespace std::literals;

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
            SPDLOG_ERROR("[sol2] Unhandled C++ system_error; category={} message=\"{}\" desc=\"{}\"",
                system_error.code().category().name(),
                system_error.code().message(),
                description);
        } else {
            SPDLOG_ERROR("[sol2] Unhandled C++ exception; typeid={} what=\"{}\" desc=\"{}\"",
                unmangled_name(typeid(exception)),
                exception.what(),
                description);
        }
    } else {
        SPDLOG_ERROR("[sol2] Unhandled C++ exception; desc=\"{}\"", description);
    }
    return sol::stack::push(L, description);
}

namespace lua::impl {
    std::string repr(sol::object);
}

void engine::Game::setup_lua()
{
    m_lua = sol::state {};
    m_lua.set_exception_handler(&exception_handler);

    m_lua.open_libraries(sol::lib::base);
    // m_lua.open_libraries(sol::lib::package); disabled
    m_lua.open_libraries(sol::lib::coroutine);
    m_lua.open_libraries(sol::lib::string);
    // m_lua.open_libraries(sol::lib::os); disabled
    m_lua.open_libraries(sol::lib::math);
    m_lua.open_libraries(sol::lib::table);
    m_lua.open_libraries(sol::lib::debug);
    m_lua.open_libraries(sol::lib::bit32);
    // m_lua.open_libraries(sol::lib::io); disabled
    m_lua.open_libraries(sol::lib::ffi);
    m_lua.open_libraries(sol::lib::jit);
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

    sol::environment sv_env(m_lua, sol::create, m_lua.globals());
    m_lua.safe_script_file("lua/sv_init.lua"s, sv_env, [](lua_State *, sol::protected_function_result pfr) {
        SPDLOG_ERROR("Failed to run server script: {} {}", sol::to_string(pfr.status()), pfr.get<sol::error>().what());
        return pfr;
    });

    sol::environment cl_env(m_lua, sol::create, m_lua.globals());
    m_lua.safe_script_file("lua/cl_init.lua"s, cl_env, [](lua_State *, sol::protected_function_result pfr) {
        SPDLOG_ERROR("Failed to run client script: {} {}", sol::to_string(pfr.status()), pfr.get<sol::error>().what());
        return pfr;
    });
}