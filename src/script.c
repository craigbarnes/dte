#include "script.h"
#include "error.h"

#ifdef USE_LUA

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "common.h"
#include "edit.h"

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

void script_state_init(void)
{
    lua_State *L = luaL_newstate();
    if (L == NULL) {
        fputs("luaL_newstate() failed; insufficient memory\n", stderr);
        exit(1);
    }
    luaL_openlibs(L);
    luaL_requiref(L, "editor", luaopen_editor, true);
    BUG_ON(lua_gettop(L) != 1);
    BUG_ON(!lua_istable(L, 1));
    lua_pop(L, 1);
    script_state = L;
}

void cmd_lua(const char* UNUSED(pf), char **args)
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

void cmd_lua_file(const char* UNUSED(pf), char **args)
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

void cmd_lua(const char* UNUSED(pf), char** UNUSED(args))
{
    no_lua_error("lua");
}

void cmd_lua_file(const char* UNUSED(pf), char** UNUSED(args))
{
    no_lua_error("lua-file");
}

#endif
