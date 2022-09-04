


#include "stdafx.h"

#include "relabel.h"
#include "peg.h"

#include "jass_parser.hpp"


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



void init_config() {

	fs::path path = fs::current_path() / "locale" / "zh-CN" / "parser.lng";

	if (!fs::exists(path)) {
		return;
	}
	std::ifstream file(path);

	if (!file.is_open()) {
		return;
	}

	std::string line, key;
	while (std::getline(file, line)) {
		std::smatch result;
		if (std::regex_match(line, result, std::regex("^\\[(.+)\\]$"))) {
			key = result[1];
		}
		else if (!key.empty()) {
			std::string& old = jass_error_map[key];
			old += "\n" + line;
		}
	}

	file.close();

	for (auto&& [k, v] : jass_error_map) {
		//将旧版本的格式化代码 修改成新版本的

		v = std::regex_replace(v, std::regex("%(d)"), "{:$1}");
		v = std::regex_replace(v, std::regex("%s"), "{}");
	}

	for (auto&& [k, v] : jass_error_map) {
		//std::cout << k << ":" << v << std::endl;
	}
}

//void test(sol::state& lua, sol::table& parser) {
//	sol::table defs = lua.create_table();
//	sol::table mt = lua.create_table();
//	defs[sol::metatable_key] = mt;
//
//	mt["__index"] = [&](sol::variadic_args aargs) {
//		sol::table self = aargs[0];
//		std::string key = aargs[1];
//
//		self[key] = [&](sol::variadic_args args) {
//			sol::function result = parser[key];
//			if (result.valid()) {
//				return result(args);
//			}
//		};
//		return self[key];
//	};
//
//}



int main(int argn, char** argv) {

	std::locale::global(std::locale("zh_CN.UTF-8"));

	init_config();

	sol::state lua;

	lua.open_libraries();

	lua.require("ffi", luaopen_ffi);
	lua.require("lpeglabel", luaopen_lpeglabel);

	lua.require_script("relabel", relabel_script, false, "relabel");
	lua.require_script("peg", peg_script, false, "peg");


	std::string script = R"(globals 
	integer a = 0
	integer b = 0
	integer c = 0
endglobals)";
	ParseResult result;
	jass_parser(lua, script, result);
	
	//printf("%i\n", std::stoi("-1a", 0, 16));

	//lua.script("print(require('ffi'))");
	//lua.script("print(require('lpeglabel'))");
	//lua.script("print(require('relabel'))");

	//test(lua);
	
	return 0;
}