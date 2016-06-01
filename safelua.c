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

struct Handler {
    safelua_Handler callback;
    void *udata;
};

struct Policy {
    struct Allocator *allocator;
    struct Handler *handlers;
    size_t nb_handlers, size_handlers;
};

lua_State *safelua_open(void)
{
    struct Policy *policy = malloc(sizeof(struct Policy));
    policy->allocator = new_allocator();
    policy->handlers = NULL;
    policy->nb_handlers = policy->size_handlers = 0;

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

static void free_resources(struct Policy *policy, int why)
{
    size_t i;
    for(i = 0; i < policy->nb_handlers; ++i)
        policy->handlers[i].callback(why, policy->handlers[i].udata);
    free(policy->handlers);
}

void safelua_close(lua_State *state)
{
    struct Policy *policy = getudregistry(state, REG_POLICY);
    lua_close(state);
    free_resources(policy, SAFELUA_FINISHED);
    delete_allocator(policy->allocator);
    free(policy);
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

    /* If catch point is already set, do a normal pcallk */
    if(getudregistry(state, REG_CANCELJMP) != NULL)
        return lua_pcallk(state, nargs, nresults, errfunc, ctx, k);
    else
    {
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
            free_resources(policy, SAFELUA_CANCELED);
            delete_allocator(policy->allocator);
            free(policy);
            ret = SAFELUA_CANCELED;
        }

        return ret;
    }
}

void safelua_checkcancel(lua_State *state)
{
    if(safelua_shouldcancel(state))
        safelua_cancel(state);
}

void safelua_cancel(lua_State *state)
{
    jmp_buf *env = getudregistry(state, REG_CANCELJMP);
    if(env)
        longjmp(*env, 1);
}

int safelua_shouldcancel(lua_State *state)
{
    safelua_CancelCheck cancel = getudregistry(state, REG_CANCEL);
    void *canceludata = getudregistry(state, REG_CANCELUDATA);

    return cancel && cancel(state, canceludata);
}

void safelua_add_handler(lua_State *state, safelua_Handler handler,
                         void *handlerudata)
{
    struct Policy *policy = getudregistry(state, REG_POLICY);
    if(++policy->nb_handlers > policy->size_handlers)
    {
        if(policy->size_handlers == 0)
            policy->size_handlers = 4;
        else
            policy->size_handlers *= 2;
        policy->handlers = realloc(
            policy->handlers,
            sizeof(struct Handler) * policy->size_handlers);
    }
    {
        struct Handler *h = &policy->handlers[policy->nb_handlers - 1];
        h->callback = handler;
        h->udata = handlerudata;
    }
}

int safelua_remove_handler(lua_State *state, safelua_Handler handler,
                           void *handlerudata)
{
    struct Policy *policy = getudregistry(state, REG_POLICY);
    size_t found = 0;
    size_t pos = 0;
    for(; pos < policy->nb_handlers; ++pos)
    {
        if(policy->handlers[pos].callback == handler &&
                policy->handlers[pos].udata == handlerudata)
            found++;
        else
            policy->handlers[pos - found] = policy->handlers[pos];
    }
    policy->nb_handlers -= found;
    return found;
}
