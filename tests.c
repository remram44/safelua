#include <stdio.h>
#include <stdlib.h>
#include <lua.h>        /* Core Lua */
#include <lauxlib.h>    /* Utility C functions */
#include <lualib.h>     /* Lua standard library */

#include "alloc.h"

lua_State *open(void)
{
    /* Create state with allocator */
    lua_State *state = lua_newstate(l_alloc, new_allocator());

    /* Open standard libraries */
    luaL_requiref(state, "", luaopen_base, 0);
    luaL_requiref(state, "coroutine", luaopen_coroutine, 1);
    luaL_requiref(state, "package", luaopen_package, 1);
    luaL_requiref(state, "string", luaopen_string, 1);
    luaL_requiref(state, "table", luaopen_table, 1);
    luaL_requiref(state, "math", luaopen_math, 1);
    luaL_requiref(state, "bit32", luaopen_bit32, 1);
    luaL_requiref(state, "io", luaopen_io, 1);
    luaL_requiref(state, "os", luaopen_os, 1);
    /* Don't open 'debug' */

    /* base */
    /*
    dofile
    loadfile
    print
    */
    /* package */
    /*
    require
    loadlib
    */
    /* string */
    /*
    find, gmatch, gsub, match
    */
    /* io */
    /*
    input, lines, open, output, popen, tmpfile
    (close, read, write)
    */
    /* os */
    /*
    execute
    exit
    remove
    rename
    setlocale
    tmpname
    */

    return state;
}

void close(lua_State *state)
{
    void *ud;
    lua_getallocf(state, &ud);
    lua_close(state);
    delete_allocator((struct Allocator *)ud);
}

int main(int argc, char **argv)
{
    lua_State *state = open();
    int i;
    for(i = 1; i < argc; ++i)
    {
        luaL_loadstring(state, argv[i]);
        if(lua_pcall(state, 0, LUA_MULTRET, 0) != LUA_OK)
        {
            printf("Error executing line %d\n", i);
            close(state);
            return 1;
        }
    }
    close(state);
    return 0;
}
