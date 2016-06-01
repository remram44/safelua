#ifndef SAFELUA_H
#define SAFELUA_H

#define SAFELUA_CANCELED    (-2)
#define SAFELUA_FINISHED    (-1)

/** Open a new Lua state that can be canceled */
lua_State *safelua_open(void);
/** Close the Lua state and free resources */
void safelua_close(lua_State *state);

/** Type for the cancel callback, used to check if this thread should stop */
typedef int (*safelua_CancelCheck)(lua_State *, void *);

/** Type for the handler callback, used to free some resource */
typedef void (*safelua_Handler)(int, void *);

/**
 * Call Lua code that is allowed to be aborted.
 *
 * This works the same as lua_pcallk(), except that if the 'cancel' callback
 * returns true, execution will abort and resources will be freed.
 *
 * If cancellation occurs, this will return SAFELUA_CANCELED and the Lua state
 * has already been freed. Other errors will leave the state in a usable state
 * (and it should be freed with safelua_close()).
 */
int safelua_pcallk(lua_State *state, int nargs, int nresults,
                   int errfunc, int ctx, lua_CFunction k,
                   safelua_CancelCheck cancel, void *canceludata);

/**
 * Call the cancel callback, and abort execution if it returns true.
 *
 * Long-running C functions should call this periodically to be abortable. Note
 * that if execution ends here, this call never returns, and the stack is lost;
 * any resource that should be released (such as memory not obtained through
 * Lua's allocator) should be registered with safelua_add_handler() before
 * calling safelua_checkcancel().
 *
 * Another option is to call safelua_shouldcancel(), and if it returns true,
 * free resources then call safelua_cancel().
 */
void safelua_checkcancel(lua_State *state);

/**
 * Cancel execution now.
 *
 * This can only be called from a frame below safelua_pcallk(). Calling it
 * while no code is running (or not with safelua_pcallk()) will do nothing;
 * calling it from another thread will probably crash.
 *
 * Only call this function from a C function call by Lua code.
 */
void safelua_cancel(lua_State *state);

/**
 * Call the cancel callback, but don't abort if it returns true.
 *
 * If true is returned, you should probably call safelua_cancel() as soon as
 * possible (after freeing locally acquired resources).
 */
int safelua_shouldcancel(lua_State *state);

/**
 * Register a resource to be freed when execution ends.
 *
 * If execution is aborted or finished, the given function will be called with
 * the provided userdata. The first argument given to that function will be
 * SAFELUA_CANCELED if execution was aborted abruptly, or SAFELUA_FINISHED if
 * the Lua state is being freed normally through safelua_close().
 */
void safelua_add_handler(lua_State *state, safelua_Handler handler,
                         void *handlerudata);

/**
 * Un-register a resource handler.
 *
 * This removes the pair (handler, handlerudata) from the list.
 *
 * @return true if the pair was registered.
 */
int safelua_remove_handler(lua_State *state, safelua_Handler handler,
                           void *handlerudata);

#endif
