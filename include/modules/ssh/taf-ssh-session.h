#ifndef MODULE_SSH2_SESSION_H
#define MODULE_SSH2_SESSION_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <libssh2.h>

typedef struct {
    int sock_fd;
    LIBSSH2_SESSION *session;
    LIBSSH2_CHANNEL *channel;
    int is_connected;
    int is_authenticated;
    int is_shell_open;
} ssh_session_t;

int l_module_ssh_session_abstract(lua_State *L);
int l_module_ssh_session_banner_get(lua_State *L);
int l_module_ssh_session_banner_set(lua_State *L);
int l_module_ssh_session_block_directions(lua_State *L);
int l_module_ssh_session_callback_set(lua_State *L);
int l_module_ssh_session_disconnect(lua_State *L);
int l_module_ssh_session_disconnect_ex(lua_State *L);
int l_module_ssh_session_flag(lua_State *L);
int l_module_ssh_session_free(lua_State *L);
int l_module_ssh_session_get_blocking(lua_State *L);
int l_module_ssh_session_get_timeout(lua_State *L);
int l_module_ssh_session_handshake(lua_State *L);
int l_module_ssh_session_hostkey(lua_State *L);
int l_module_ssh_session_init(lua_State *L);
int l_module_ssh_session_init_ex(lua_State *L);
int l_module_ssh_session_last_errno(lua_State *L);
int l_module_ssh_session_last_error(lua_State *L);
int l_module_ssh_session_method_pref(lua_State *L);
int l_module_ssh_session_methods(lua_State *L);
int l_module_ssh_session_set_blocking(lua_State *L);
int l_module_ssh_session_set_last_error(lua_State *L);
int l_module_ssh_session_set_timeout(lua_State *L);
int l_module_ssh_session_startup(lua_State *L);
int l_module_ssh_session_supported_algs(lua_State *L);

#endif // MODULE_SSH2_SESSION_H

