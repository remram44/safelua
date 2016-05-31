#include <setjmp.h>
#include <stdlib.h>
#include <lua.h>        /* Core Lua */
#include <lauxlib.h>    /* Utility C functions */
#include <lualib.h>     /* Lua standard library */

#include "alloc.h"
#include "safelua.h"

const char *const REG_POLICY = "safelua_policy";
const char *const REG_CANCEL = "safelua_cancel";
const char *const REG_CANCELUDATA = "safelua_canceludata";
const char *const REG_CANCELJMP = "safelua_canceljmp";

#define setudregistry(state, value, name) do { \
        lua_State *state_ = (state); \
        lua_pushlightuserdata(state_, (value)); \
        lua_setfield(state_, LUA_REGISTRYINDEX, (name)); \
    } while(0)

static void *getudregistry(lua_State *state, const char *name)
{
    void *ret;
    lua_getfield(state, LUA_REGISTRYINDEX, name);
    ret = lua_touserdata(state, -1);
    lua_pop(state, 1);
    return ret;
}

struct Policy {
    struct Allocator *allocator;
};

lua_State *safelua_open(void)
{
    struct Policy *policy = malloc(sizeof(struct Policy));
    policy->allocator = new_allocator();

    /* Create state with allocator */
    lua_State *state = lua_newstate(l_alloc, policy->allocator);

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

    setudregistry(state, policy, REG_POLICY);

    return state;
}

void safelua_close(lua_State *state)
{
    struct Policy *policy = getudregistry(state, REG_POLICY);
    lua_close(state);
    delete_allocator(policy->allocator);
    free(policy);
}

void safelua_checkcancel(lua_State *state)
{
    safelua_CancelCheck cancel = getudregistry(state, REG_CANCEL);
    void *canceludata = getudregistry(state, REG_CANCELUDATA);

    if(cancel(state, canceludata))
    {
        jmp_buf *env = getudregistry(state, REG_CANCELJMP);
        longjmp(*env, 1);
    }
}

static void cancel_hook(lua_State *state, lua_Debug *ar)
{
    safelua_checkcancel(state);
}

int safelua_pcallk(lua_State *state, int nargs, int nresults,
                   int errfunc, int ctx, lua_CFunction k,
                   safelua_CancelCheck cancel, void *canceludata)
{
    struct Policy *policy = getudregistry(state, REG_POLICY);
    jmp_buf env;
    int ret;

    /* Set hook from which we will be able to exit */
    lua_sethook(state, cancel_hook, LUA_MASKCOUNT, 10);

    if(setjmp(env) == 0)
    {
        setudregistry(state, &env, REG_CANCELJMP);
        setudregistry(state, cancel, REG_CANCEL);
        setudregistry(state, canceludata, REG_CANCELUDATA);
        ret = lua_pcallk(state, nargs, nresults, errfunc, ctx, k);
        lua_pushnil(state);
        lua_setfield(state, LUA_REGISTRYINDEX, REG_CANCEL);
        lua_pushnil(state);
        lua_setfield(state, LUA_REGISTRYINDEX, REG_CANCELJMP);
        lua_sethook(state, cancel_hook, 0, 0);
    }
    else
    {
        delete_allocator(policy->allocator);
        free(policy);
        ret = SAFELUA_CANCELED;
    }

    return ret;
}
