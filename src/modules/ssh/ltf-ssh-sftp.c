#include "modules/ssh/ltf-ssh-sftp.h"

#include "modules/ssh/ltf-ssh-lib.h"
#include "modules/ssh/ltf-ssh-session.h"

#include "internal_logging.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <stdlib.h>

#define DEFAULT_CHUNK_SIZE 4096

int l_module_ssh_sftp_init(lua_State *L) {
    l_ssh_session_t *s = luaL_checkudata(L, 1, SSH_SESSION_MT);
    if (!s->session) {
        luaL_error(L, "l_module_ssh_sftp_init() failed because "
                      "session was not initialized");
        return 0;
    }

    l_sftp_session_t *u = lua_newuserdata(L, sizeof *u);
    u->sftp_session = libssh2_sftp_init(s->session);
    u->sftp_handle = NULL;
    u->session = s->session;
    if (u->sftp_session == NULL) {
        u->session = NULL;
        luaL_error(L, "libssh2_sftp_init failed");
        return 0;
    }
    luaL_getmetatable(L, SFTP_SESSION_MT);
    lua_setmetatable(L, -2);
    return 1;
}

int l_module_ssh_sftp_open(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        luaL_error(L, "l_module_ssh_sftp_open() failed because "
                      "sftp_session was not initialized");

        return 0;
    }
    if (u->sftp_handle) {
        luaL_error(L, "l_module_ssh_sftp_open() failed because "
                      "sftp_handle for this sftp_session already exist");

        return 0;
    }
    if (!u->session) {
        luaL_error(L, "l_module_ssh_sftp_open() failed because "
                      "session was not initialized");

        return 0;
    }

    const char *filename = luaL_checkstring(L, 2);
    unsigned long flags = luaL_checknumber(L, 3);
    long mode = luaL_checknumber(L, 4);
    int open_type = luaL_checkinteger(L, 5);

    u->sftp_handle = libssh2_sftp_open_ex(u->sftp_session, filename,
                                          (unsigned int)strlen(filename), flags,
                                          mode, open_type);

    if (!u->sftp_handle) {
        int rc = libssh2_sftp_last_error(u->sftp_session);
        luaL_error(L, "libssh2_sftp_open() failed with code: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    return 0;
}

int l_module_ssh_sftp_read(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        luaL_error(L, "l_module_ssh_sftp_read() failed because "
                      "sftp_session was not initialized");
        return 0;
    }
    if (!u->sftp_handle) {
        luaL_error(L, "l_module_ssh_sftp_read() failed because "
                      "sftp_handle was not initialized");
        return 0;
    }
    if (!u->session) {
        luaL_error(L, "l_module_ssh_sftp_read() failed because "
                      "session was not initialized");
        return 0;
    }
    /* size argument optional */
    lua_Integer v = luaL_optinteger(L, 2, DEFAULT_CHUNK_SIZE);
    if (v < 0) {
        luaL_error(L, "l_module_ssh_sftp_read() failed: size is negative");
        return 0;
    }

    /* cap size to some reasonable max to avoid huge mallocs */
    size_t len = (size_t)v;
    if (len == 0)
        len = DEFAULT_CHUNK_SIZE;
    const size_t MAX_CHUNK = 64 * 1024 * 1024; // 64MB safety cap
    if (len > MAX_CHUNK)
        len = MAX_CHUNK;

    char *buf = (char *)malloc(len);
    if (!buf) {
        luaL_error(L, "l_module_ssh_sftp_read() failed: out of memory");
        return 0;
    }

    ssize_t rc = libssh2_sftp_read(u->sftp_handle, buf, len);

    if (rc > 0) {
        lua_pushlstring(L, buf, (size_t)rc);
        free(buf);
        return 1;
    } else if (rc == 0) {
        /* EOF */
        lua_pushlstring(L, "", 0);
        free(buf);
        return 1;
    } else {
        /* rc < 0 : error code */
        free(buf);
        const char *msg = NULL;
        msg = ssh_err_to_str(rc);
        if (msg && msg[0]) {
            luaL_error(L, "l_module_ssh_sftp_read() failed with code %d: %s",
                       rc, msg);
        } else {
            luaL_error(L, "l_module_ssh_sftp_read() failed with code %d", rc);
        }
        return 0;
    }
}

int l_module_ssh_sftp_write(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        luaL_error(L, "l_module_ssh_sftp_write() failed because "
                      "sftp_session was not initialized");
        return 0;
    }
    if (!u->sftp_handle) {
        luaL_error(L, "l_module_ssh_sftp_write() failed because "
                      "sftp_handle was not initialized");
        return 0;
    }
    if (!u->session) {
        luaL_error(L, "l_module_ssh_sftp_write() failed because "
                      "session was not initialized");
        return 0;
    }
    size_t len;
    const char *s = luaL_checklstring(L, 2, &len);

    int v = luaL_checkinteger(L, 3);
    if (v < 0) {
        luaL_error(L, "l_module_ssh_sftp_write() failed because "
                      "argument 3 is negative");
        return 0;
    }

    if (len < (size_t)v) {
        luaL_error(L, "l_module_ssh_sftp_write() failed because size of "
                      "string is biger then 3rd argument");
        return 0;
    }

    int rc = libssh2_sftp_write(u->sftp_handle, s, len);

    if (rc < 0) {
        int err = libssh2_sftp_last_error(u->sftp_session);
        libssh2_sftp_close(u->sftp_handle);
        u->sftp_handle = NULL;
        luaL_error(L, "l_module_ssh_sftp_write() failed with code: %d %s", err,
                   ssh_err_to_str(err));
        return 0;
    }

    // No error and full buf was written
    lua_pushinteger(L, rc);
    return 1;
}

int l_module_ssh_sftp_close(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        return 0;
    }
    if (!u->sftp_handle) {
        return 0;
    }
    if (!u->session) {
        return 0;
    }

    libssh2_sftp_close(u->sftp_handle);

    u->sftp_handle = NULL;

    return 0;
}

int l_module_ssh_sftp_shutdown(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u->sftp_session) {
        return 0;
    }
    if (u->sftp_handle) {
        return 0;
    }
    if (!u->session) {
        return 0;
    }
    libssh2_sftp_shutdown(u->sftp_session);

    u->sftp_session = NULL;
    u->session = NULL;

    return 0;
}

static void normalize_abs_path_inplace(char *s) {
    if (!s || s[0] != '/')
        return;

    // Read from a copy so we don't clobber unread input while writing output.
    char *in = strdup(s);
    if (!in)
        return; // best-effort

    size_t seg_starts_cap = 64;
    size_t seg_starts_len = 0;
    size_t *seg_starts = (size_t *)malloc(seg_starts_cap * sizeof(size_t));
    if (!seg_starts) {
        free(in);
        return; // best-effort
    }

    size_t r = 1; // read index in "in"
    size_t w = 1; // write index in "s"
    s[0] = '/';

    while (in[r]) {
        while (in[r] == '/')
            r++;
        if (!in[r])
            break;

        size_t seg_start = r;
        while (in[r] && in[r] != '/')
            r++;
        size_t seg_len = r - seg_start;

        // "."
        if (seg_len == 1 && in[seg_start] == '.') {
            continue;
        }

        // ".."
        if (seg_len == 2 && in[seg_start] == '.' && in[seg_start + 1] == '.') {
            if (seg_starts_len > 0) {
                w = seg_starts[--seg_starts_len];
            } else {
                w = 1; // stay at "/"
            }
            continue;
        }

        // ensure capacity for stack push
        if (seg_starts_len == seg_starts_cap) {
            size_t new_cap = seg_starts_cap * 2;
            size_t *p = (size_t *)realloc(seg_starts, new_cap * sizeof(size_t));
            if (!p)
                break; // best-effort
            seg_starts = p;
            seg_starts_cap = new_cap;
        }

        // If not root, add separator.
        if (w > 1)
            s[w++] = '/';

        // Record rewind point (start of this segment including the separator we
        // just added). Rewinding to this removes the last segment cleanly.
        seg_starts[seg_starts_len++] = (w > 1) ? (w - 1) : 1;

        // Copy segment
        memcpy(s + w, in + seg_start, seg_len);
        w += seg_len;
    }

    // terminate
    if (w == 0) {
        s[0] = '/';
        w = 1;
    }
    s[w] = '\0';

    // remove trailing slash unless root
    if (w > 1 && s[w - 1] == '/') {
        s[w - 1] = '\0';
    }

    free(seg_starts);
    free(in);
}

static char *join_abs_lexical(const char *base_abs, const char *rel) {
    // base_abs must start with '/'
    if (!base_abs || base_abs[0] != '/' || !rel)
        return NULL;

    size_t base_len = strlen(base_abs);
    size_t rel_len = strlen(rel);

    // base + "/" + rel + NUL (but avoid double slash)
    bool need_slash = (base_len > 1 && base_abs[base_len - 1] != '/');

    size_t need = base_len + (need_slash ? 1 : 0) + rel_len + 1;
    char *out = (char *)malloc(need);
    if (!out)
        return NULL;

    if (need_slash) {
        snprintf(out, need, "%s/%s", base_abs, rel);
    } else {
        snprintf(out, need, "%s%s", base_abs, rel);
    }

    if (rel[0] == '/') {
        char *out = strdup(rel);
        if (!out)
            return NULL;
        normalize_abs_path_inplace(out);
        return out;
    }

    return out;
}

/* Fetch remote cwd as an absolute path string.
 * Uses libssh2_sftp_realpath(".", ...). This can canonicalize cwd, but we only
 * use it as a base. Returns malloc'd NUL-terminated string.
 */
static char *sftp_get_cwd_abs(LIBSSH2_SFTP *sftp) {
    char buf[PATH_MAX];
    int n = libssh2_sftp_realpath(sftp, ".", buf, (int)sizeof(buf) - 1);
    if (n < 0)
        return NULL;
    buf[n] = '\0';

    // Ensure it starts with '/', some servers might return relative-ish (rare).
    // We'll force.
    if (buf[0] != '/') {
        // best-effort: prefix with '/'
        size_t need = strlen(buf) + 2;
        char *out = (char *)malloc(need);
        if (!out)
            return NULL;
        snprintf(out, need, "/%s", buf);
        normalize_abs_path_inplace(out);
        return out;
    }

    char *out = strdup(buf);
    if (!out)
        return NULL;
    normalize_abs_path_inplace(out);
    return out;
}

static const char *sftp_mode_to_type(unsigned long perm) {
    if (LIBSSH2_SFTP_S_ISREG(perm))
        return "file";
    if (LIBSSH2_SFTP_S_ISDIR(perm))
        return "directory";
    if (LIBSSH2_SFTP_S_ISLNK(perm))
        return "symlink";
    return NULL;
}

int l_module_ssh_sftp_file_info(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u || !u->sftp_session) {
        luaL_error(L, "sftp_session not initialized");
        return 0;
    }

    const char *path = luaL_checkstring(L, 2);

    LIBSSH2_SFTP_ATTRIBUTES a;
    memset(&a, 0, sizeof(a));

    int rc =
        libssh2_sftp_stat_ex(u->sftp_session, path, (unsigned int)strlen(path),
                             LIBSSH2_SFTP_LSTAT, /* DO NOT follow symlinks */
                             &a);

    if (rc < 0) {
        unsigned long fx = libssh2_sftp_last_error(u->sftp_session);
        if (fx == LIBSSH2_FX_NO_SUCH_FILE || fx == LIBSSH2_FX_NO_SUCH_PATH) {
            lua_pushnil(L);
            return 1;
        }
        luaL_error(L, "libssh2_sftp_stat_ex(LSTAT) error: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    if (!(a.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS)) {
        luaL_error(L, "SFTP server did not return permissions for '%s'", path);
        return 0;
    }

    const char *type = sftp_mode_to_type(a.permissions);
    if (!type) {
        luaL_error(L, "Unknown type of remote file for '%s' (perm=%lo)", path,
                   a.permissions);
        return 0;
    }

    /* Compute absolute path WITHOUT resolving the file itself */
    char *abs = NULL;
    if (path[0] == '/') {
        abs = strdup(path);
        if (!abs) {
            luaL_error(L, "out of memory");
            return 0;
        }
        normalize_abs_path_inplace(abs);
    } else {
        char *cwd = sftp_get_cwd_abs(u->sftp_session);
        if (!cwd) {
            luaL_error(L, "libssh2_sftp_realpath('.') failed (cannot build "
                          "absolute path)");
            return 0;
        }
        abs = join_abs_lexical(cwd, path);
        free(cwd);
        if (!abs) {
            luaL_error(L, "out of memory");
            return 0;
        }
    }

    lua_newtable(L);

    lua_pushstring(L, type);
    lua_setfield(L, -2, "type");

    lua_pushstring(L, abs);
    lua_setfield(L, -2, "path");

    /* size: best-effort; for symlink servers vary (may be link-text length or
     * 0) */
    lua_Integer size = 0;
    if (a.flags & LIBSSH2_SFTP_ATTR_SIZE) {
        size = (lua_Integer)a.filesize;
    }
    lua_pushinteger(L, size);
    lua_setfield(L, -2, "size");

    lua_Integer perms = 0;
    if (a.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
        perms = (lua_Integer)(a.permissions);
    }
    lua_pushinteger(L, perms);
    lua_setfield(L, -2, "permissions");

    free(abs);
    return 1;
}

int l_module_ssh_sftp_resolve_symlink(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (!u || !u->sftp_session) {
        luaL_error(L, "sftp_session not initialized");
        return 0;
    }

    const char *path = luaL_checkstring(L, 2);

    // 1) Confirm the symlink itself exists and is a symlink (LSTAT = no follow)
    LIBSSH2_SFTP_ATTRIBUTES a;
    memset(&a, 0, sizeof(a));

    int rc =
        libssh2_sftp_stat_ex(u->sftp_session, path, (unsigned int)strlen(path),
                             LIBSSH2_SFTP_LSTAT, &a);

    if (rc < 0) {
        unsigned long fx = libssh2_sftp_last_error(u->sftp_session);
        // "symlink does not exist" -> error
        if (fx == LIBSSH2_FX_NO_SUCH_FILE || fx == LIBSSH2_FX_NO_SUCH_PATH) {
            luaL_error(L, "symlink does not exist: '%s'", path);
            return 0;
        }
        luaL_error(L, "libssh2_sftp_stat_ex(LSTAT) error: %s",
                   ssh_err_to_str(rc));
        return 0;
    }

    if (!(a.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) ||
        !LIBSSH2_SFTP_S_ISLNK(a.permissions)) {
        luaL_error(L, "not a symlink: '%s'", path);
        return 0;
    }

    // 2) Resolve it (realpath follows symlinks). If target missing => nil.
    char buf[PATH_MAX];
    rc =
        libssh2_sftp_realpath(u->sftp_session, path, buf, (int)sizeof(buf) - 1);
    if (rc < 0) {
        unsigned long fx = libssh2_sftp_last_error(u->sftp_session);
        // symlink exists but target doesn't -> nil
        if (fx == LIBSSH2_FX_NO_SUCH_FILE || fx == LIBSSH2_FX_NO_SUCH_PATH) {
            lua_pushnil(L);
            return 1;
        }
        luaL_error(L, "libssh2_sftp_realpath('%s') failed: %s", path,
                   ssh_err_to_str(rc));
        return 0;
    }

    lua_pushlstring(L, buf, (size_t)rc);
    return 1;
}

/* ---------- DESTRUCTOR (GC) ---------- */
int l_sftp_session_gc(lua_State *L) {
    l_sftp_session_t *u = luaL_checkudata(L, 1, SFTP_SESSION_MT);
    if (u && u->sftp_session) {
        libssh2_sftp_close(u->sftp_handle);
        libssh2_sftp_shutdown(u->sftp_session);
        u->sftp_session = NULL;
        u->sftp_handle = NULL;
        u->session = NULL;
    }
    return 0;
}
/* ---------- REGISTRATION ---------- */
static const luaL_Reg sftp_methods[] = {
    {"open", l_module_ssh_sftp_open},
    {"write", l_module_ssh_sftp_write},
    {"read", l_module_ssh_sftp_read},
    {"file_info", l_module_ssh_sftp_file_info},
    {"resolve_symlink", l_module_ssh_sftp_resolve_symlink},
    {"close", l_module_ssh_sftp_close},
    {"shutdown", l_module_ssh_sftp_shutdown},
    {NULL, NULL}};

int l_module_register_ssh_sftp(lua_State *L) {
    LOG("Registering ltf-ssh-sftp");

    if (luaL_newmetatable(L, SFTP_SESSION_MT)) {
        lua_newtable(L);
        luaL_setfuncs(L, sftp_methods, 0);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_sftp_session_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pop(L, 1);

    LOG("Successfully registered ltf-ssh-sftp");
    return 0;
}
