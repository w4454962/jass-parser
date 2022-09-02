


#include "stdafx.h"

extern "C" {
	int luaopen_ffi(lua_State* L);
	int luaopen_lpeglabel(lua_State* L);
};



std::unordered_map<std::string, std::string> jass_error_map;



std::string_view convert_message(std::string_view msg) {
	if (jass_error_map.find(msg.data()) != jass_error_map.end()) {
		return jass_error_map.at(msg.data());
	}
	return msg;
}




int main(int argn, char** argv) {

	std::locale::global(std::locale("zh_CN.UTF-8"));

	sol::state lua;


	lua.open_libraries(sol::lib::base, sol::lib::package);

	lua["package"]["preload"]["ffi"] = luaopen_ffi;
	lua["package"]["preload"]["lpeglabel"] = luaopen_lpeglabel;


	lua.script("print(1)");

	lua.script("print(require('ffi'))");
	lua.script("print(require('lpeglabel'))");


	return 0;
}