#include "modules/proc/taf-proc.h"

#include "internal_logging.h"

#include "util/lua.h"

#include <errno.h>
#include <signal.h>
#include <spawn.h>
#include <string.h>

extern char **environ;

static void close_fd(int fd) {
    if (fd >= 0)
        close(fd);
}

static void proc_close_streams(l_module_proc_t *p) {
    LOG("Closing streams...");
    if (!p) {
        LOG("Proc is NULL");
        return;
    }
    if (p->in) {
        fclose(p->in);
        p->in = NULL;
        LOG("Closed stdin file");
    }
    if (p->out) {
        fclose(p->out);
        p->out = NULL;
        LOG("Closed stdout file");
    }
    if (p->err) {
        fclose(p->err);
        p->err = NULL;
        LOG("Closed stderr file");
    }

    LOG("Closing file descriptors...");
    for (size_t i = 0; i < 2; i++) {
        close_fd(p->pin[i]);
        close_fd(p->pout[i]);
        close_fd(p->perr[i]);
    }
    LOG("Successfully closed streams...");
}

static int proc_kill(l_module_proc_t *p, int signo) {
    LOG("Killing process...");
    return (p && p->pid > 0) ? kill(p->pid, signo) : -1;
}

int l_module_proc_spawn(lua_State *L) {
    LOG("Invoked taf-proc spawn...");
    int s = selfshift(L);

    luaL_checktype(L, s, LUA_TTABLE);
    size_t len = lua_rawlen(L, s);
    if (len == 0) {
        LOG("argv table is empty");
        return luaL_error(L, "argv table is empty");
    }
    LOG("argv size: %zu", len);
    char **argv = malloc(sizeof(*argv) * (len + 1));
    if (!argv) {
        LOG("Out of memory.");
        return luaL_error(L, "taf-proc:run: Out of memory");
    }
    for (size_t i = 0; i < len; i++) {
        lua_geti(L, s, i + 1);
        size_t str_len;
        const char *str = luaL_checklstring(L, -1, &str_len);
        argv[i] = strndup(str, str_len);
        if (!argv[i]) {
            LOG("Out of memory.");
            return luaL_error(L, "taf-proc:run: Out of memory");
        }
        LOG("argv[%zu]: %s", i, argv[i]);
        lua_pop(L, 1);
    }
    argv[len] = NULL;

    l_module_proc_t *proc = lua_newuserdata(L, sizeof *proc);
    memset(proc, 0, sizeof *proc);

    LOG("Piping...");

    if (pipe(proc->pin) || pipe(proc->pout) || pipe(proc->perr)) {
        const char *err = strerror(errno);
        LOG(" Unable to pipe(): %s", err);
        proc_close_streams(proc);
        return luaL_error(L, "pipe(): %s", err);
    }

    LOG("Spawning file actions...");
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);

    posix_spawn_file_actions_adddup2(&fa, proc->pin[0], STDIN_FILENO);
    posix_spawn_file_actions_adddup2(&fa, proc->pout[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&fa, proc->perr[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&fa, proc->pin[1]);
    posix_spawn_file_actions_addclose(&fa, proc->pout[0]);
    posix_spawn_file_actions_addclose(&fa, proc->perr[0]);

    LOG("Spawning...");
    int rc = posix_spawnp(&proc->pid, argv[0], &fa, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&fa);

    LOG("Freeing argv...");
    for (size_t i = 0; i < len; i++)
        free(argv[i]);
    free(argv);

    if (rc) {
        const char *err = strerror(rc);
        LOG("Unable to posix_spawnp(): %s", err);
        proc_close_streams(proc);
        return luaL_error(L, "posix_spawnp(): %s", err);
    }

    LOG("Closing child side ends...");
    close_fd(proc->pin[0]);
    close_fd(proc->pout[1]);
    close_fd(proc->perr[1]);

    LOG("Opening file descriptors...");
    proc->in = fdopen(proc->pin[1], "w");
    proc->out = fdopen(proc->pout[0], "r");
    proc->err = fdopen(proc->perr[0], "r");

    if (!proc->in || !proc->out || !proc->err) {
        LOG("fdopen failed");
        proc_kill(proc, SIGKILL);
        proc_close_streams(proc);
        return luaL_error(L, "fdopen() failed");
    }

    luaL_getmetatable(L, "taf-proc");
    lua_setmetatable(L, -2);

    LOG("Successfully finished taf-proc run.");
    return 1;
}

int l_module_proc_write(lua_State *L) {
    LOG("Invoked taf-proc write...");
    int s = selfshift(L);
    l_module_proc_t *proc = luaL_checkudata(L, s, "taf-proc");
    LOG("Proc pointer: %p", (void *)proc);
    size_t len;
    const char *buf = luaL_checklstring(L, s + 1, &len);
    LOG("Buffer to write: %.*s", (int)len, buf);

    if (!proc->in) {
        LOG("stdin closed");
        return luaL_error(L, "stdin closed");
    }
    size_t wr = fwrite(buf, 1, len, proc->in);
    fflush(proc->in);

    LOG("Wrote %zu bytes", wr);
    lua_pushinteger(L, (lua_Integer)wr);

    LOG("Successfully finished taf-proc write.");
    return 1;
}

int l_module_proc_read(lua_State *L) {
    LOG("Invoked taf-proc read …");
    int s = selfshift(L);
    l_module_proc_t *proc = luaL_checkudata(L, s, "taf-proc");
    LOG("Proc pointer: %p", (void *)proc);

    lua_Integer want = luaL_optinteger(L, s + 1, 4096);
    if (want <= 0)
        want = 4096;

    const char *which = luaL_optstring(L, s + 2, "stdout");

    LOG("Stream = %s , want = %lld", which, want);

    FILE **Fptr = NULL;
    if (strcasecmp(which, "stdout") == 0)
        Fptr = &proc->out;
    else if (strcasecmp(which, "stderr") == 0)
        Fptr = &proc->err;
    else
        return luaL_error(L, "unknown stream '%s'", which);

    if (!*Fptr) { // already closed / EOF
        LOG("%s already closed", which);
        lua_pushliteral(L, "");
        return 1;
    }

    if (proc->pid == 0) {
        LOG("Process exited, draining %s to EOF …", which);
        luaL_Buffer lb;
        luaL_buffinit(L, &lb);

        char tmp[4096];
        size_t got;
        while ((got = fread(tmp, 1, sizeof tmp, *Fptr)) > 0)
            luaL_addlstring(&lb, tmp, got);

        fclose(*Fptr);
        *Fptr = NULL;
        luaL_pushresult(&lb);
        LOG("Drain complete (%llu bytes)", lua_rawlen(L, -1));
        return 1;
    }

    char *tmp = lua_newuserdatauv(L, want, 0);
    size_t got = fread(tmp, 1, want, *Fptr);
    LOG("Got %zu bytes", got);
    lua_pushlstring(L, tmp, got);

    LOG("Successfully finished taf-proc read.");
    return 1;
}

int l_module_proc_wait(lua_State *L) {
    LOG("Invoked taf-proc wait...");
    int s = selfshift(L);
    l_module_proc_t *proc = luaL_checkudata(L, s, "taf-proc");
    LOG("Proc pointer: %p", (void *)proc);
    if (proc->pid <= 0) {
        LOG("Proc pid is %d <= 0", proc->pid);
        lua_pushnil(L);
        LOG("Successfully finished taf-proc wait.");
        return 1;
    }

    int st;
    pid_t r = waitpid(proc->pid, &st, WNOHANG);
    if (r == 0) {
        LOG("Proc is running.");
        lua_pushnil(L);
        LOG("Successfully finished taf-proc wait.");
        return 1;
    }
    if (r == -1) {
        const char *err = strerror(errno);
        LOG("waitpid: %s", err);
        return luaL_error(L, "waitpid: %s", err);
    }

    proc->pid = 0;

    if (WIFEXITED(st)) {
        int status = WEXITSTATUS(st);
        LOG("Exited with status %d", status);
        lua_pushinteger(L, status);
    } else if (WIFSIGNALED(st)) {
        int signal = WTERMSIG(st);
        LOG("Exited with signal %d (%d)", 128 + signal, signal);
        lua_pushinteger(L, signal);
    } else {
        LOG("Unknown termination status.");
        lua_pushinteger(L, -1);
    }

    LOG("Successfully finished taf-proc wait.");

    return 1;
}

int l_module_proc_kill(lua_State *L) {
    LOG("Invoked taf-proc kill...");
    int s = selfshift(L);
    l_module_proc_t *proc = lua_touserdata(L, s);
    LOG("Proc pointer: %p", (void *)proc);

    if (proc) {
        proc_close_streams(proc);
        proc_kill(proc, SIGTERM);
        l_module_proc_wait(L);
    }

    LOG("Successfully finished taf-proc kill.");
    return 0;
}

/*----------- registration ------------------------------------------*/
static int l_gc(lua_State *L) { return l_module_proc_kill(L); }

static const luaL_Reg proc_fns[] = {
    {"read", l_module_proc_read},   //
    {"write", l_module_proc_write}, //
    {"wait", l_module_proc_wait},   //
    {"kill", l_module_proc_kill},   //
    {"__gc", l_gc},                 //
    {NULL, NULL},                   //
};

static const luaL_Reg module_fns[] = {
    {"spawn", l_module_proc_spawn}, //
    {NULL, NULL},                   //
};

int l_module_proc_register_module(lua_State *L) {
    LOG("Registering taf-proc module...");

    luaL_newmetatable(L, "taf-proc");

    LOG("Registering proc functions...");
    lua_newtable(L);
    luaL_setfuncs(L, proc_fns, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
    LOG("Proc functions registered.");

    LOG("Registering module functions...");
    lua_newtable(L);
    luaL_setfuncs(L, module_fns, 0);
    LOG("Module functions registered.");

    LOG("Successfully registered taf-proc module.");
    return 1;
}
