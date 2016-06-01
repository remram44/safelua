#include <assert.h>
#include <stdio.h>
#include <lua.h>        /* Core Lua */
#include <lauxlib.h>    /* Utility C functions */

#include "safelua.h"

int mycancel(lua_State *_state, void *ud)
{
    int *counter = ud;
    fprintf(stderr, "Hook: %d\n", *counter);
    return --(*counter) <= 0;
}

char resources[10] = "          ";
size_t resources_nb = 0;

static void free_resource(int why, void *resource)
{
    assert(*(char*)resource != ' ');
    fprintf(stderr, "Handler freed resource %d: %c\n",
            (int)((char*)resource - resources), *(char*)resource);
    *(char*)resource = ' ';
}

static int l_alloc(lua_State *state)
{
    size_t len;
    const char *str;
    if(lua_gettop(state) != 1)
        luaL_error(state, "Expected a single argument");
    str = luaL_checklstring (state, 1, &len);
    if(len != 1)
        luaL_error(state, "Expected 1-character string");
    resources[resources_nb] = str[0];
    lua_pushlightuserdata(state, &resources[resources_nb]);
    fprintf(stderr, "Allocated resource %d: %c\n", (int)resources_nb, str[0]);
    safelua_add_handler(state, free_resource, &resources[resources_nb]);
    resources_nb++;
    return 1;
}

static int l_free(lua_State *state)
{
    char *ptr;
    if(lua_gettop(state) != 1)
        luaL_error(state, "Expected a single argument");
    ptr = lua_touserdata(state, 1);
    if(ptr == NULL)
        luaL_error(state, "Invalid resource");
    assert(*ptr != ' ');
    fprintf(stderr, "Freed resource %d: %c\n", (int)(ptr - resources), *ptr);
    assert(safelua_remove_handler(state, free_resource, ptr) == 1);
    *ptr = ' ';
    return 0;
}

int main(int argc, char **argv)
{
    lua_State *state = safelua_open();
    int i;
    lua_pushcfunction(state, l_alloc);
    lua_setglobal(state, "alloc");
    lua_pushcfunction(state, l_free);
    lua_setglobal(state, "free");
    for(i = 1; i < argc; ++i)
    {
        luaL_loadstring(state, argv[i]);
        int counter = 3;
        int ret = safelua_pcallk(state, 0, LUA_MULTRET, 0, 0, NULL,
                                 mycancel, &counter);
        if(ret == LUA_OK)
            ;
        else if(ret == SAFELUA_CANCELED)
        {
            printf("Canceled during line %d\n", i);
            return 2;
        }
        else
        {
            printf("Error executing line %d: %d\n", i, ret);
            safelua_close(state);
            return 1;
        }
    }
    safelua_close(state);
    fprintf(stderr, "Resources: \"%s\"\n", resources);
    return 0;
}
