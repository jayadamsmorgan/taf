#ifndef MODULE_JSON_H
#define MODULE_JSON_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/******************* API START ***********************/

// json:serialize(value:table) -> string
int l_module_json_serialize(lua_State *L);

// json:deserialize(str:string) -> table
int l_module_json_deserialize(lua_State *L);

/******************* API END *************************/

// Register "taf-json" module
int l_module_json_register_module(lua_State *L);

#endif // MODULE_JSON_H
