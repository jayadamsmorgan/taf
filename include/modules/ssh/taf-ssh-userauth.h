#ifndef MODULE_SSH2_USERAUTH_H
#define MODULE_SSH2_USERAUTH_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <libssh2.h>

int l_module_ssh_userauth_authenticated(lua_State *l);
int l_module_ssh_userauth_hostbased_fromfile(lua_State *l);
int l_module_ssh_userauth_hostbased_fromfile_ex(lua_State *l);
int l_module_ssh_userauth_keyboard_interactive(lua_State *l);
int l_module_ssh_userauth_keyboard_interactive_ex(lua_State *l);
int l_module_ssh_userauth_list(lua_State *l);
int l_module_ssh_userauth_password(lua_State *l);
int l_module_ssh_userauth_password_ex(lua_State *l);
int l_module_ssh_userauth_publickey(lua_State *l);
int l_module_ssh_userauth_publickey_fromfile(lua_State *l);
int l_module_ssh_userauth_publickey_fromfile_ex(lua_State *l);
int l_module_ssh_userauth_publickey_frommemory(lua_State *l);

#endif // MODULE_SSH2_USERAUTH_H
