#ifndef TAF_WEBDRIVER_H
#define TAF_WEBDRIVER_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

void module_web_close_all_sessions();

int l_module_web_register_module(lua_State *L);

#endif // TAF_WEBDRIVER_H
