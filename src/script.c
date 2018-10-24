#include "script.h"
#include "error.h"

#ifdef USE_LUA

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "config.h"
#include "edit.h"
#include "options.h"
#include "util/macros.h"

static lua_State *script_state;

static const char hook_table_name[] = "event_hooks";

static const char *const event_names[] = {
    "set-file-options",
    NULL
};

static_assert(ARRAY_COUNT(event_names) == 1 + NR_EVENT_HOOK_TYPES);

static int add_event_hook(lua_State *L)
{
    EventHookType event_type = luaL_checkoption(L, 1, NULL, event_names);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    if (lua_getfield(L, LUA_REGISTRYINDEX, hook_table_name) != LUA_TTABLE) {
        luaL_error(L, "%s table missing", hook_table_name);
    }

    lua_Integer idx = 1;
    if (lua_geti(L, -1, event_type) != LUA_TTABLE) {
        lua_pop(L, 1); // Pop the nil
        lua_createtable(L, 1, 0);
    } else {
        idx += luaL_len(L, -1);
    }

    // registry[event_type][idx] = func
    lua_pushvalue(L, 2);
    lua_seti(L, -2, idx);
    lua_seti(L, -2, event_type);

    return 0;
}

static int editor_info_msg(lua_State *L)
{
    info_msg("%s", luaL_checkstring(L, 1));
    return 0;
}

static int editor_insert(lua_State *L)
{
    size_t size;
    const char *text = luaL_checklstring(L, 1, &size);
    insert_text(text, size);
    return 0;
}

static int editor_set_local_option(lua_State *L)
{
    const char *option = luaL_checkstring(L, 1);
    const char *value;

    switch (lua_type(L, 2)) {
    case LUA_TSTRING:
    case LUA_TNUMBER:
        value = lua_tostring(L, 2);
        break;
    case LUA_TBOOLEAN:
        value = lua_toboolean(L, 2) ? "true" : "false";
        break;
    default:
        luaL_checktype(L, 2, LUA_TSTRING);
        return 0;
    }

    set_option(option, value, true, false);
    return 0;
}

static const luaL_Reg editorlib[] = {
    {"add_hook", add_event_hook},
    {"info_msg", editor_info_msg},
    {"insert", editor_insert},
    {"set_local_option", editor_set_local_option},
    {NULL, NULL}
};

static int luaopen_editor(lua_State *L)
{
    luaL_newlib(L, editorlib);
    return 1;
}

static void openlibs(lua_State *L)
{
    static const luaL_Reg libs[] = {
        {"_G", luaopen_base},
        {"package", luaopen_package},
        {"coroutine", luaopen_coroutine},
        {"table", luaopen_table},
        {"io", luaopen_io},
        {"os", luaopen_os},
        {"string", luaopen_string},
        {"math", luaopen_math},
        {"utf8", luaopen_utf8},
        {"editor", luaopen_editor},
    };
    for (size_t i = 0; i < ARRAY_COUNT(libs); i++) {
        luaL_requiref(L, libs[i].name, libs[i].func, true);
        lua_pop(L, 1);
    }
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

    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, hook_table_name);
}

void run_builtin_script(const char *name)
{
    // Strip "=" prefix; see docs for `lua_Debug::source` for details
    // https://www.lua.org/manual/5.3/manual.html#lua_Debug
    const char *const builtin_name = name + (name[0] == '=' ? 1 : 0);

    const BuiltinConfig *cfg = get_builtin_config(builtin_name);
    if (!cfg) {
        error_msg (
            "Error reading '%s': no built-in script exists for that path",
            builtin_name
        );
    }
    const char *text = cfg->text.data;
    const size_t length = cfg->text.length;

    lua_State *L = script_state;
    if (
        luaL_loadbufferx(L, text, length, name, "t") != LUA_OK
        || lua_pcall(L, 0, 0, 0) != LUA_OK
    ) {
        error_msg("%s", lua_tostring(L, -1));
        lua_pop(L, -1);
    }
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

void run_event_hooks(EventHookType event_type)
{
    lua_State *L = script_state;

    int hook_table_type = lua_getfield(L, LUA_REGISTRYINDEX, hook_table_name);
    if (unlikely(hook_table_type != LUA_TTABLE)) {
        error_msg("%s table missing", hook_table_name);
        return;
    }

    if (lua_geti(L, -1, event_type) != LUA_TTABLE) {
        lua_pop(L, 1);
        return;
    }

    for (lua_Integer i = 1, n = luaL_len(L, -1); i <= n; i++) {
        if (unlikely(lua_geti(L, -1, i) != LUA_TFUNCTION)) {
            lua_pop(L, 1);
            continue;
        }
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            error_msg (
                "%s['%s'][%" PRIdMAX "]: %s",
                hook_table_name,
                event_names[event_type],
                (intmax_t) i,
                lua_tostring(L, -1)
            );
            // Pop error value pushed by lua_pcall()
            lua_pop(L, 1);
        }
    }

    lua_pop(L, 2);
}

#else

void script_state_init(void) {}
void run_event_hooks(EventHookType UNUSED_ARG(event_type)) {}
void run_builtin_script(const char* UNUSED_ARG(name)) {}

#define NO_LUA_MSG "editor was built without Lua scripting support"

void cmd_lua(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    error_msg("lua: " NO_LUA_MSG);
}

void cmd_lua_file(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    error_msg("lua-file: " NO_LUA_MSG);
}

#endif
