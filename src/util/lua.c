#include "util/lua.h"

int lua_table_is_array(lua_State *L, int index, lua_Integer *max_out) {
    lua_Integer max = 0;
    int array_like = 1;

    lua_pushnil(L); /* first key */
    while (lua_next(L, index)) {
        if (lua_type(L, -2) == LUA_TNUMBER && lua_isinteger(L, -2)) {
            lua_Integer k = lua_tointeger(L, -2);
            if (k > max)
                max = k;
        } else {
            array_like = 0; /* non-integer key */
        }
        lua_pop(L, 1); /* pop value keep key */
    }
    if (max_out)
        *max_out = max;
    return array_like && max > 0;
}

json_object *lua_to_json(lua_State *L, int index) {
    int t = lua_type(L, index);
    switch (t) {
    case LUA_TNIL:
        return NULL;

    case LUA_TBOOLEAN:
        return json_object_new_boolean(lua_toboolean(L, index));

    case LUA_TNUMBER:
        if (lua_isinteger(L, index)) {
            lua_Integer vi = lua_tointeger(L, index);

            /* choose 32- or 64-bit helper depending on range */
            if (vi >= INT32_MIN && vi <= INT32_MAX)
                return json_object_new_int((int)vi);
            else
                return json_object_new_int64((int64_t)vi);
        } else {
            lua_Number vd = lua_tonumber(L, index);
            return json_object_new_double((double)vd);
        }

    case LUA_TSTRING:
        return json_object_new_string(lua_tostring(L, index));

    case LUA_TTABLE: {
        /* Decide array vs object */
        lua_Integer max;
        int is_array = lua_table_is_array(L, index, &max);

        json_object *j =
            is_array ? json_object_new_array() : json_object_new_object();

        lua_pushnil(L);
        while (lua_next(L, index)) {
            /* stack: key at -2, value at -1 */
            json_object *child = lua_to_json(L, lua_gettop(L));

            if (is_array) {
                json_object_array_put_idx(
                    j, lua_tointeger(L, -2) - 1, /* 0-based */
                    child ? child : json_object_new_null());
            } else {
                const char *kstr = lua_tostring(L, -2);
                if (!kstr)
                    kstr = luaL_tolstring(L, -2, NULL); /* stringify key */
                json_object_object_add(j, kstr,
                                       child ? child : json_object_new_null());
                if (lua_type(L, -2) != LUA_TSTRING)
                    lua_pop(L, 1); /* popped tolstring */
            }

            lua_pop(L, 1); /* pop value, keep key for next iteration */
        }
        return j;
    }

    default:
        return json_object_new_null(); /* unsupported → null */
    }
}

void json_to_lua(lua_State *L, struct json_object *obj) {
    if (!obj || json_object_is_type(obj, json_type_null)) {
        lua_pushnil(L);
        return;
    }

    switch (json_object_get_type(obj)) {
    case json_type_boolean:
        lua_pushboolean(L, json_object_get_boolean(obj));
        break;

    case json_type_double:
        lua_pushnumber(L, json_object_get_double(obj));
        break;

    case json_type_int:
        lua_pushnumber(L, json_object_get_int64(obj));
        break;

    case json_type_string:
        lua_pushstring(L, json_object_get_string(obj));
        break;

    case json_type_array: {
        size_t len = json_object_array_length(obj);
        lua_createtable(L, (int)len, 0);
        for (size_t i = 0; i < len; ++i) {
            struct json_object *child = json_object_array_get_idx(obj, i);
            json_to_lua(L, child);
            lua_seti(L, -2, (lua_Integer)i + 1);
        }
        break;
    }

    case json_type_object: {
        lua_createtable(L, 0, json_object_get_object(obj)->count);

        json_object_iter iter;
        json_object_object_foreachC(obj, iter) {
            json_to_lua(L, iter.val);      /* push value     */
            lua_setfield(L, -2, iter.key); /* t[key] = value */
        }
        break;
    }

    default: /* json_type_tokener_error or future types */
        lua_pushnil(L);
        break;
    }
}
