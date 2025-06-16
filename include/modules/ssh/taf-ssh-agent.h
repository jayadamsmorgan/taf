#ifndef MODULE_SSH2_AGENT_H
#define MODULE_SSH2_AGENT_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <libssh2.h>

typedef struct {
    LIBSSH2_AGENT *agent;
    LIBSSH2_SESSION *session;
    const LIBSSH2_AGENT_PUBLICKEY *current_identity;
    void *identity_cursor;
} ssh_agent_t; // SSH-Agent for key authentication

int l_module_ssh_agent_init(lua_State *L);
int l_module_ssh_agent_connect(lua_State *L);
int l_module_ssh_agent_disconnect(lua_State *L);
int l_module_ssh_agent_free(lua_State *L);
int l_module_ssh_agent_list_identities(lua_State *L);
int l_module_ssh_agent_userauth(lua_State *L);

#endif // MODULE_SSH2_H
