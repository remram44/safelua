#include <stdio.h>
#include <stdlib.h>
#include <lua.h>        /* Core Lua */
#include <lauxlib.h>    /* Utility C functions */
#include <lualib.h>     /* Lua standard library */

static void *l_simple_alloc(void *ud, void *ptr, size_t osize,
                            size_t nsize)
{
    (void)ud; (void)osize;  /* not used */
    if(nsize == 0)
    {
        free(ptr);
        return NULL;
    }
    else
        return realloc(ptr, nsize);
}

lua_State *open(void)
{
    return lua_newstate(l_simple_alloc, NULL);
}

int main(int argc, char **argv)
{
    lua_State *state = open();
    luaL_openlibs(state);   /* Open all standard libraries */
    for(int i = 1; i < argc; ++i)
    {
        luaL_loadstring(state, argv[i]);
        if(lua_pcall(state, 0, LUA_MULTRET, 0) != LUA_OK)
        {
            printf("Error executing line %d\n", i);
            return 1;
        }
    }
    return 0;
}
