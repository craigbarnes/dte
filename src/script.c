#include "script.h"
#include "error.h"

#ifdef USE_LUA

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "edit.h"
#include "util/macros.h"

static lua_State *script_state;

static int insert(lua_State *L)
{
    size_t size;
    const char *text = luaL_checklstring(L, 1, &size);
    insert_text(text, size);
    return 0;
}

static const luaL_Reg editorlib[] = {
    {"insert", insert},
    {NULL, NULL}
};

static int luaopen_editor(lua_State *L)
{
    luaL_newlib(L, editorlib);
    return 1;
}

static int os_setlocale(lua_State *L)
{
    // Locale should be set at editor startup and never touched again.
    // It's almost always wrong for a library to call setlocale(3), so
    // only broken Lua libraries will be affected by this.
    return luaL_error(L, "setlocale() not allowed");
}

static void openlibs(lua_State *L) {
    static const luaL_Reg libs[] = {
        {"_G", luaopen_base},
        {"package", luaopen_package},
        {"coroutine", luaopen_coroutine},
        {"table", luaopen_table},
        {"io", luaopen_io},
        {"string", luaopen_string},
        {"math", luaopen_math},
        {"utf8", luaopen_utf8},
        {"editor", luaopen_editor},
    };
    for (size_t i = 0; i < ARRAY_COUNT(libs); i++) {
        luaL_requiref(L, libs[i].name, libs[i].func, true);
        lua_pop(L, 1);
    }

    luaL_requiref(L, "os", luaopen_io, true);
    lua_pushliteral(L, "setlocale");
    lua_pushcfunction(L, os_setlocale);
    lua_rawset(L, -3);
    lua_pop(L, 1);
}

void script_state_init(void)
{
    lua_State *L = luaL_newstate();
    if (L == NULL) {
        errno = ENOMEM;
        perror("luaL_newstate");
        exit(1);
    }
    openlibs(L);
    script_state = L;
}

void cmd_lua(const char* UNUSED_ARG(pf), char **args)
{
    const char *const text = *args;
    lua_State *L = script_state;
    if (
        luaL_loadbufferx(L, text, strlen(text), "=cmdline", "t") != LUA_OK
        || lua_pcall(L, 0, 0, 0) != LUA_OK
    ) {
        error_msg("lua: %s", lua_tostring(L, -1));
        lua_pop(L, -1);
    }
}

void cmd_lua_file(const char* UNUSED_ARG(pf), char **args)
{
    const char *const filename = *args;
    lua_State *L = script_state;
    if (
        luaL_loadfilex(L, filename, "t") != LUA_OK
        || lua_pcall(L, 0, 0, 0) != LUA_OK
    ) {
        error_msg("lua-file: %s", lua_tostring(L, -1));
        lua_pop(L, -1);
    }
}

#else

void script_state_init(void) {}

static void no_lua_error(const char *cmd)
{
    error_msg("%s: editor was built without Lua scripting support", cmd);
}

void cmd_lua(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    no_lua_error("lua");
}

void cmd_lua_file(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    no_lua_error("lua-file");
}

#endif
