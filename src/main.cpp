


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
		} else if (!key.empty()) {
			std::string& old = jass_error_map[key];
			if (!old.empty() || line.empty()) {
				old += "\n";
			}
			old += line;
		}
	}

	file.close();

	for (auto&& [k, v] : jass_error_map) {
		//将旧版本的格式化代码 修改成新版本的

		v = std::regex_replace(v, std::regex("%(d)"), "{:$1}");
		v = std::regex_replace(v, std::regex("%s"), "{}");
	}

	//for (auto&& [k, v] : jass_error_map) {
	//	std::cout << k << ":" << v << std::endl;
	//}
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


	std::string script = R"(

type agent			    extends     handle

type ttt extends integer

native test0 takes nothing returns nothing 

native test1 takes integer a returns integer 

native test2 takes integer a, integer b returns integer 

constant native test3 takes integer a returns ttt
 

globals 
	integer a = 1 + 2 * 3 / 4
	string b = "hello"
	boolean c 

	integer d = test1(a)

endglobals

function code1 takes nothing returns nothing 
	local integer num = test1(1)
	local integer array num2  
	local integer num3 = ((1 + 3) * 4) / 5 + 1

	set num = test1(1) + 20 * 30 + test1(1) + num * test1(num)

	set num2[1+1] = num + num2[num+num2[test1(1)]]


endfunction

function code2 takes nothing returns nothing 
	
endfunction

)";
	ParseResult result;
	jass_parser(lua, script, result);
	
	
	for (auto& v : result.log.errors) {
		std::cout << "[error]:" << v->file << ":" << v->line << ": " << v->message << "\n" << v->at_line << std::endl;
	}
	
	lua.script("print('finish')");


	//printf("%i\n", std::stoi("-1a", 0, 16));

	//lua.script("print(require('ffi'))");
	//lua.script("print(require('lpeglabel'))");
	//lua.script("print(require('relabel'))");

	//test(lua);
	
	return 0;
}