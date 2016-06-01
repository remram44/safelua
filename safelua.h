#ifndef SAFELUA_H
#define SAFELUA_H

#define SAFELUA_CANCELED    (-2)
#define SAFELUA_FINISHED    (-1)

lua_State *safelua_open(void);
void safelua_close(lua_State *state);

typedef int (*safelua_CancelCheck)(lua_State *, void *);

typedef void (*safelua_Handler)(int, void *);

int safelua_pcallk(lua_State *state, int nargs, int nresults,
                   int errfunc, int ctx, lua_CFunction k,
                   safelua_CancelCheck cancel, void *canceludata);

void safelua_checkcancel(lua_State *state);

void safelua_cancel(lua_State *state);

int safelua_shouldcancel(lua_State *state);

void safelua_add_handler(lua_State *state, safelua_Handler handler,
                         void *handlerudata);

int safelua_remove_handler(lua_State *state, safelua_Handler handler,
                           void *handlerudata);

#endif
