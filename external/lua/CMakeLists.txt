
FetchContent_Declare(lua
    URL https://www.lua.org/ftp/lua-5.4.0.tar.gz
)
FetchContent_GetProperties(lua)
if (NOT lua_POPULATED)
    FetchContent_Populate(lua)
    # lua.hpp uses extern "C" even when Lua its compiled using C++
    file(REMOVE ${lua_SOURCE_DIR}/src/lua.hpp)
    set(lua_public_headers
        ${lua_SOURCE_DIR}/src/lauxlib.h
        ${lua_SOURCE_DIR}/src/lualib.h
        ${lua_SOURCE_DIR}/src/lua.h
    )
    set(lua_private_headers
        ${lua_SOURCE_DIR}/src/lapi.h
        ${lua_SOURCE_DIR}/src/lcode.h
        ${lua_SOURCE_DIR}/src/lctype.h
        ${lua_SOURCE_DIR}/src/ldebug.h
        ${lua_SOURCE_DIR}/src/ldo.h
        ${lua_SOURCE_DIR}/src/lfunc.h
        ${lua_SOURCE_DIR}/src/lgc.h
        ${lua_SOURCE_DIR}/src/ljumptab.h
        ${lua_SOURCE_DIR}/src/llex.h
        ${lua_SOURCE_DIR}/src/llimits.h
        ${lua_SOURCE_DIR}/src/lmem.h
        ${lua_SOURCE_DIR}/src/lobject.h
        ${lua_SOURCE_DIR}/src/lopcodes.h
        ${lua_SOURCE_DIR}/src/lopnames.h
        ${lua_SOURCE_DIR}/src/lparser.h
        ${lua_SOURCE_DIR}/src/lprefix.h
        ${lua_SOURCE_DIR}/src/lstate.h
        ${lua_SOURCE_DIR}/src/lstring.h
        ${lua_SOURCE_DIR}/src/ltable.h
        ${lua_SOURCE_DIR}/src/ltm.h
        ${lua_SOURCE_DIR}/src/luaconf.h
        ${lua_SOURCE_DIR}/src/lundump.h
        ${lua_SOURCE_DIR}/src/lvm.h
        ${lua_SOURCE_DIR}/src/lzio.h
    )
    set(lua_private_sources
        ${lua_SOURCE_DIR}/src/lapi.c
        ${lua_SOURCE_DIR}/src/lauxlib.c
        ${lua_SOURCE_DIR}/src/lbaselib.c
        ${lua_SOURCE_DIR}/src/lcode.c
        ${lua_SOURCE_DIR}/src/lcorolib.c
        ${lua_SOURCE_DIR}/src/lctype.c
        ${lua_SOURCE_DIR}/src/ldblib.c
        ${lua_SOURCE_DIR}/src/ldebug.c
        ${lua_SOURCE_DIR}/src/ldo.c
        ${lua_SOURCE_DIR}/src/ldump.c
        ${lua_SOURCE_DIR}/src/lfunc.c
        ${lua_SOURCE_DIR}/src/lgc.c
        ${lua_SOURCE_DIR}/src/linit.c
        ${lua_SOURCE_DIR}/src/liolib.c
        ${lua_SOURCE_DIR}/src/llex.c
        ${lua_SOURCE_DIR}/src/lmathlib.c
        ${lua_SOURCE_DIR}/src/lmem.c
        ${lua_SOURCE_DIR}/src/loadlib.c
        ${lua_SOURCE_DIR}/src/lobject.c
        ${lua_SOURCE_DIR}/src/lopcodes.c
        ${lua_SOURCE_DIR}/src/loslib.c
        ${lua_SOURCE_DIR}/src/lparser.c
        ${lua_SOURCE_DIR}/src/lstate.c
        ${lua_SOURCE_DIR}/src/lstring.c
        ${lua_SOURCE_DIR}/src/lstrlib.c
        ${lua_SOURCE_DIR}/src/ltable.c
        ${lua_SOURCE_DIR}/src/ltablib.c
        ${lua_SOURCE_DIR}/src/ltm.c
        ${lua_SOURCE_DIR}/src/lua.c
        ${lua_SOURCE_DIR}/src/luac.c
        ${lua_SOURCE_DIR}/src/lundump.c
        ${lua_SOURCE_DIR}/src/lutf8lib.c
        ${lua_SOURCE_DIR}/src/lvm.c
        ${lua_SOURCE_DIR}/src/lzio.c
    )

    add_library(lua)
    # if we compile with C++ we can use exceptions instead of longjmp
    # still not ideal but anything is better than longjmp
    set_source_files_properties(${lua_private_sources} PROPERTIES LANGUAGE CXX) 
    target_sources(lua PUBLIC ${lua_public_headers} PRIVATE ${lua_private_headers} ${lua_private_sources})
    target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)
    if (UNIX)
        target_link_libraries(lua PRIVATE dl)
        # target_compile_definitions(lua PUBLIC LUA_USE_DLOPEN)
    endif()
    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
        target_compile_definitions(lua PUBLIC LUA_USE_LINUX)
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
        target_compile_definitions(lua PUBLIC LUA_USE_LINUX)
        # Needed if you want to use readline
        # target_include_directories(lua PRIVATE "/usr/include/edit")
        # target_link_libraries(lua PRIVATE edit)
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_compile_definitions(lua PUBLIC LUA_USE_MACOSX)
    else()
        message(WARNING "Using posix platform for lua")
        target_compile_definitions(lua PUBLIC LUA_USER_POSIX)
    endif()
endif()

