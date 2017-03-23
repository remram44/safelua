[![Build Status](https://travis-ci.org/remram44/safelua.svg?branch=master)](https://travis-ci.org/remram44/safelua)

safelua
=======

This project aims to make Lua completely safe to use with untrusted code, without requiring OS-level isolation mechanisms. Furthermore, it makes the Lua script interruptible (i.e. from another thread, or after a maximum execution time expires) without any memory or resource leak, without using a separate process.

Note that this is not a fork of Lua, but builds on top of the standard Lua distribution (so you can still use your system's library).

* A hook allows a running script to be terminated asynchronously before the next Lua instruction
* A custom allocator is used to allow all the memory for the Lua state to be reclaimed
* Overwrites of some parts of the standard library to keep track of resources (i.e. open files) so they can be reclaimed on interruption; also allows to restrict what the script is allowed to do

Of course this requires cooperation from any C module that you allow the script to load. This project provides a framework to make your own extensions interruptible and leak-free.

How to use
==========

Instead of doing:

```c
lua_State *L = luaL_newstate();
luaL_openlibs(L);
```

Do something like:

```c
lua_State *L = safelua_open(&policy);
```

The provided policy object holds a whitelist for which libraries are to be loaded, allows you to kill the running script asynchronously (from another thread, or any called C function), and can filter fine-grained requests such as which file can be open, how much memory can be used, etc.
