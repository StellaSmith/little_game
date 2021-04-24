

#include <fmt/format.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>

#include "engine/game.hpp"

using json = nlohmann::json;
using namespace std::literals;

extern json g_config_engine;
extern bool g_verbose;

static std::string_view l_tostring(lua_State *L, int n) noexcept
{
    size_t len;
    char const *buf = lua_tolstring(L, n, &len);
    return { buf, len };
}

static void l_pack(lua_State *L, int n) noexcept
{
    int t = lua_gettop(L);
    lua_createtable(L, n, 0);
    for (int i = 1; i <= t; ++i)
        lua_seti(L, t - n, i);
}

static int l_unpack(lua_State *L, int idx) noexcept
{
    idx = lua_absindex(L, idx);
    int n = 0;
    for (int i = 1;; ++i) {
        if (lua_geti(L, idx, i) == LUA_TNIL) break;
    }
    lua_pop(L, 1);
    return n - 1;
}

int engine::Game::l_print(lua_State *L)
{
    auto self = reinterpret_cast<engine::Game *>(lua_touserdata(L, lua_upvalueindex(1)));
    std::uint32_t max_lines = lua_tointeger(L, lua_upvalueindex(2));

    std::string line;

    int n = lua_gettop(L);

    for (int i = 1; i <= n; ++i) {
        switch (lua_type(L, i)) {
        case LUA_TSTRING:
            line += l_tostring(L, i);
            break;
        case LUA_TBOOLEAN:
            line += lua_toboolean(L, i) ? "true"sv : "false"sv;
            break;
        case LUA_TNIL:
            line += "nil"sv;
            break;
        case LUA_TNUMBER:
            fmt::format_to(std::back_inserter(line), "{}", lua_tonumber(L, i));
            break;

        default:
            switch (luaL_getmetafield(L, i, "__tostring")) {
            case LUA_TSTRING:
                line += l_tostring(L, -1);
                lua_pop(L, 1);
                break;
            case LUA_TFUNCTION: {
                lua_pushvalue(L, i);
                lua_pcall(L, 1, 1, 0);
                if (lua_type(L, -1) == LUA_TSTRING) {
                    line += l_tostring(L, -1);
                    break;
                } else
                    lua_pop(L, 1);
                [[fallthrough]];
            }

            default: {
                char const *const type_name = luaL_typename(L, i);
                auto const address = reinterpret_cast<std::uintptr_t>(lua_topointer(L, i));
                constexpr std::string_view fmt = []() {
                    if constexpr (sizeof(void *) == 8)
                        return "<{} at 0x{:016}>"sv;
                    else
                        return "<{} at 0x{:08}>"sv;
                }();
                fmt::format_to(std::back_inserter(line), fmt, type_name, address);
            } break;
            };
        }
        line += '\t';
    }

    if (n)
        line.pop_back();

    self->m_console_text.emplace_back(std::move(line));
    while (self->m_console_text.size() > max_lines)
        self->m_console_text.pop_front();
    return 0;
}

/*
** Message handler used to run all chunks
*/
static int msghandler(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);
    if (msg == NULL) { /* is error object not a string? */
        if (luaL_callmeta(L, 1, "__tostring") && /* does it have a metamethod */
            lua_type(L, -1) == LUA_TSTRING) /* that produces a string? */
            return 1; /* that is the message */
        else
            msg = lua_pushfstring(L, "(error object is a %s value)",
                luaL_typename(L, 1));
    }
    luaL_traceback(L, L, msg, 1); /* append a standard traceback */
    return 1; /* return the traceback */
}

/*
** Interface to 'lua_pcall', which sets appropriate message function
** and C-signal handler. Used to run all chunks.
*/
static int docall(lua_State *L, int narg, int nres)
{
    int status;
    int base = lua_gettop(L) - narg; /* function index */
    lua_pushcfunction(L, msghandler); /* push message handler */
    lua_insert(L, base); /* put it under function and args */
    // globalL = L; /* to be available to 'laction' */
    // signal(SIGINT, laction); /* set C-signal handler */
    status = lua_pcall(L, narg, nres, base);
    // signal(SIGINT, SIG_DFL); /* reset C-signal handler */
    lua_remove(L, base); /* remove message handler from the stack */
    return status;
}

#include <filesystem>

static int l_open_import(lua_State *L)
{
    int n = lua_gettop(L);

    lua_newtable(L); // modules table
    lua_newuserdatauv(L, 1, 1); // use userdata, so users can't modify the metatable
    lua_newtable(L);

    lua_pushvalue(L, -2);
    lua_pushcclosure(
        L, [](lua_State *L) -> int {
            luaL_checktype(L, 1, LUA_TSTRING);

            auto rel_path = l_tostring(L, 1);
            auto current_path = std::filesystem::current_path();
            auto abs_path = std::filesystem::absolute(current_path / rel_path);
            if (abs_path.native().find(current_path.native()) != 0) {
                auto s1 = abs_path.string();
                auto s2 = current_path.string();
                return luaL_error(L, "Can't import %s: %s is outside %s", rel_path.data(), s1.c_str(), s2.c_str());
            }

            lua_getiuservalue(L, lua_upvalueindex(1), 1);
            int t = lua_getfield(L, -1, abs_path.string().c_str());
            if (t == LUA_TNIL) {
                lua_newtable(L);
                lua_pushvalue(L, 1);
                lua_setfield(L, -2, "path");
                if (luaL_loadfilex(L, abs_path.string().c_str(), "t"))
                    return luaL_error(L, "%s", lua_tostring(L, -1));

                int n = lua_gettop(L);
                lua_call(L, 0, LUA_MULTRET);
                n = lua_gettop(L) - n;
                l_pack(L, n);

                lua_pushvalue(L, -1);
                lua_setfield(L, -3, "values");

                return l_unpack(L, -1);
            } else if (t == LUA_TTABLE) {
                lua_getfield(L, -1, "values");
                return l_unpack(L, -1);
            } else
                return luaL_error(L, "Module table for %s is of type %s, table or nil expected", abs_path.string().c_str(), lua_typename(L, t));
        },
        1);
    lua_setfield(L, -2, "__call");

    lua_pushvalue(L, -2);
    lua_pushcclosure(
        L, [](lua_State *L) -> int {
            if (lua_type(L, 1) == LUA_TSTRING && l_tostring(L, 1) == "modules"sv) {
                lua_getiuservalue(L, lua_upvalueindex(1), 1);
                return 1;
            }
            return 0;
        },
        1);
    lua_setfield(L, -2, "__index");

    lua_setmetatable(L, -2);
    lua_setglobal(L, "import");

    lua_settop(L, n);
    return 0;
}

void engine::Game::setup_lua()
{
    if (m_lua)
        lua_close(m_lua);
    m_lua = luaL_newstate();

    lua_pushboolean(m_lua, g_verbose);
    lua_setglobal(m_lua, "_VERBOSE");

    std::uint32_t max_lines = 5000;

    try {
        g_config_engine.at("/Terminal/max_lines"_json_pointer).get_to(max_lines);
        if (max_lines > std::numeric_limits<lua_Integer>::max())
            throw std::range_error(fmt::format("/Terminal/max_lines cant be bigger than {}", std::numeric_limits<lua_Integer>::max()));
    } catch (std::exception &e) {
        if (g_verbose)
            spdlog::error("Can't obtain /Terminal/max_lines, using the default of {}\n\t{}\n", max_lines, e.what());
    }

    luaopen_base(m_lua);

    l_open_import(m_lua);

    luaopen_math(m_lua);
    lua_setglobal(m_lua, "math");
    luaopen_string(m_lua);

    lua_pushnil(m_lua);
    lua_setfield(m_lua, -2, "dump");
    lua_setglobal(m_lua, "string");

    luaopen_utf8(m_lua);
    lua_setglobal(m_lua, "utf8");

    luaopen_coroutine(m_lua);
    lua_setglobal(m_lua, "coroutine");

    luaopen_table(m_lua);
    lua_setglobal(m_lua, "table");

    // debug, os, io, and package are disabled

    // override print
    lua_pushlightuserdata(m_lua, this);
    lua_pushinteger(m_lua, max_lines);
    lua_pushcclosure(m_lua, l_print, 2);
    lua_setglobal(m_lua, "print");

    // disable these globals
    for (auto f : { "collectgarbage", "dofile", "load", "loadfile" }) {
        lua_pushnil(m_lua);
        lua_setglobal(m_lua, f);
    }

    int ret;

    ret = luaL_loadfilex(m_lua, "lua/sv_init.lua", "t") || docall(m_lua, 0, 0);
    if (ret)
        spdlog::error("Can't init server lua: {}\n", lua_tostring(m_lua, -1));

    ret = luaL_loadfilex(m_lua, "lua/cl_init.lua", "t") || docall(m_lua, 0, 0);
    if (ret)
        spdlog::error("Can't init client lua: {}\n", lua_tostring(m_lua, -1));
}