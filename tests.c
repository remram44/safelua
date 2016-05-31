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

int main(int argc, char **argv)
{
    lua_State *state = safelua_open();
    int i;
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
    return 0;
}
