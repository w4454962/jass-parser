


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



bool tests(sol::state& lua, const fs::path& tests_path) {

	std::cout << "each path " << tests_path << std::endl;

	if (!fs::exists(tests_path)) {
		return 0;
	}

	std::vector<fs::path> paths;

	for (const auto i : fs::recursive_directory_iterator(tests_path)) {
		if (i.is_regular_file() && (i.path().extension() == ".j" || i.path().extension() == ".J")) {
			paths.push_back(i.path());
		}
	}

	//paths = { fs::path("E:\\jass-parser\\tests\\pjass-tests\\should-fail\\shouldnt-crash-14.j") };


	for (auto& file : paths) {
		std::ifstream stream(file, std::ios::binary);

		ParseConfig config;
		
		config.file = file.filename().string();

		config.script = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

		ParseResult result;
		bool success = jass_parser(lua, config, result);

		 
		std::string src = file.string();

		fs::path path = std::regex_replace(src, std::regex("\\.j"), ".err");

		if (!fs::exists(path)) {
			path = std::regex_replace(src, std::regex("\\.j"), ".warn");
		}

		if (result.log.errors.size() == 0) {
			if (!success) {
				std::cout << src << "\t fail" << std::endl;
				break;
			} else {
				std::cout << src << "\tpass" << std::endl;
			}
		} 
		
		if (result.log.errors.size() > 0) {
			std::cout << src << "\t error" << std::endl;

			for (auto v : result.log.errors) {
				std::cout << "[error]<" << v->message << ">" << std::endl;
			}
		}
		if (result.log.warnings.size() > 0) {
			for (auto v : result.log.warnings) {
				std::cout << "[warning]<" << v->message << ">" << std::endl;
			}
		}

		std::ifstream file(path, std::ios::binary);
		if (file.is_open()) {

			std::string file_str = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

			std::cout << "[error]{" << file_str << "}" << std::endl;
		
			file.close();
		}
		
	}

	return 1;
}


void check_script(sol::state& lua, const fs::path file, ParseResult& result) {
	ParseConfig config;

	config.file = file.filename().string();
	
	std::ifstream stream(file, std::ios::binary);

	config.script = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

	bool success = jass_parser(lua, config, result);

	auto& errors = result.log.errors;
	auto& warnings = result.log.warnings;

	if (errors.size() == 0) {
		if (!success) {
			std::cout << file << "\t fail" << std::endl;
		}
		else {
			std::cout << file << "\tpass" << std::endl;
		}
	}

	if (errors.size() > 0) {
		std::cout << file << "\t error" << std::endl;

		for (auto v : errors) {
			std::cout << "[error]<" << v->message << ">" << std::endl;
			std::cout << "{" << v->at_line << "}" << std::endl;
		}
	}

	if (warnings.size() > 0) {
		for (auto v : warnings) {
			std::cout << "[warning]<" << v->message << ">" << std::endl;
			std::cout << "{" << v->at_line << "}" << std::endl;
		}
	}
}

int main(int argn, char** argv) {

	std::locale::global(std::locale("zh_CN.UTF-8"));

	init_config();

	sol::state lua;

	lua.open_libraries();

	lua.require("ffi", luaopen_ffi);
	lua.require("lpeglabel", luaopen_lpeglabel);

	lua.require_script("relabel", relabel_script, false, "relabel");
	lua.require_script("peg", peg_script, false, "peg");

	//tests(lua, fs::path(argv[1]));
	clock_t start = clock();


	fs::path path = fs::current_path() / "war3" / "24";

	ParseResult* result = new ParseResult();

	check_script(lua, path / "common.j", *result);
	check_script(lua, path / "blizzard.j", *result);
	check_script(lua, path / "war3map.j", *result); 

	//check_script(lua, fs::current_path() / "tests" / "aa.j", *result);

	std::cout << "time : " << ((double)(clock() - start) / CLOCKS_PER_SEC) << " s" << std::endl;;

	
	return 0;
}