#ifndef MODULE_PROC_H
#define MODULE_PROC_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#if defined(_WIN32) || defined(_WIN64)
#define NOMINMAX
#include <windows.h>
typedef DWORD taf_pid_t;
#else /* POSIX */
#include <sys/types.h>
#include <unistd.h>
typedef pid_t taf_pid_t;
#endif

typedef struct {
    pid_t pid;

    FILE *in;
    FILE *out;
    FILE *err;

    int pin[2];
    int pout[2];
    int perr[2];

} l_module_proc_t;

/******************* API START ***********************/

// proc:spawn(argc:[string]) -> proc_handle
int l_module_proc_spawn(lua_State *L);

// proc:read(proc: proc_handle, want:integer=4096) -> string
int l_module_proc_read(lua_State *L);

// proc:write(proc: proc_handle, buf:string) -> integer
int l_module_proc_write(lua_State *L);

// proc:wait(proc: proc_handle) -> integer
int l_module_proc_wait(lua_State *L);

// proc:kill(proc: proc_handle)
int l_module_proc_kill(lua_State *L);

/******************* API END *************************/

// Register "taf-proc" module
int l_module_proc_register_module(lua_State *L);

#endif // MODULE_PROC_H
