#ifndef MODULE_SSH2_PUBLICKEY_H
#define MODULE_SSH2_PUBLICKEY_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <libssh2.h>

int l_module_ssh_publickey_add(lua_State *L);
int l_module_ssh_publickey_add_ex(lua_State *L);
int l_module_ssh_publickey_init(lua_State *L);
int l_module_ssh_publickey_list_fetch(lua_State *L);
int l_module_ssh_publickey_list_free(lua_State *L);
int l_module_ssh_publickey_remove(lua_State *L);
int l_module_ssh_publickey_remove_ex(lua_State *L);
int l_module_ssh_publickey_shutdown(lua_State *L);

#endif // MODULE_SSH2_PUBLICKEY_H_H
