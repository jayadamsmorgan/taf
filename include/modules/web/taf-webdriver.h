#ifndef TAF_WEBDRIVER_H
#define TAF_WEBDRIVER_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/******************* API START ***********************/

// tw:session_cmd(
//     session:session_ud,
//     method:string,
//     endpoint:string,
//     payload:table
// ) -> table? response, string? err
int l_module_web_session_cmd(lua_State *L);

// tw:session_end(session:session_ud) -> string? err
int l_module_web_session_end(lua_State *L);

// tw:session_start(
//     driver_port:integer,
//     backend:string,
//     extraflags:[string]
// ) -> session_ud?, string? err
int l_module_web_session_start(lua_State *L);

/******************* API END *************************/

// Close every WebDriver session opened
void module_web_close_all_sessions();

// Register "taf-webdriver" module
int l_module_web_register_module(lua_State *L);

#endif // TAF_WEBDRIVER_H
