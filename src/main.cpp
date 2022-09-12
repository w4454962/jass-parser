﻿#include <Windows.h>
#include <mimalloc-new-delete.h>

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

void test_file(sol::state& lua, const fs::path& file) {

	std::ifstream stream(file, std::ios::binary);

	ParseConfig config;

	config.file = file.filename().string();

	config.script = std::make_shared<std::string>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

	auto jass = std::make_shared<Jass>();

	bool success = 0;// jass_parser(lua, config, jass);

	std::string src = file.string();

	fs::path path = std::regex_replace(src, std::regex("\\.j"), ".err");

	if (!fs::exists(path)) {
		path = std::regex_replace(src, std::regex("\\.j"), ".warn");
	}

	if (jass->log.errors.size() == 0) {
		if (!success) {
			std::cout << src << "\t fail" << std::endl;
			return;
		}
		else {
			std::cout << src << "\tpass" << std::endl;
		}
	}

	if (jass->log.errors.size() > 0) {
		std::cout << src << "\t error" << std::endl;

		for (auto v : jass->log.errors) {
			std::cout << "[error]<" << v->message << ">" << std::endl;
		}
	}
	if (jass->log.warnings.size() > 0) {
		for (auto v : jass->log.warnings) {
			std::cout << "[warning]<" << v->message << ">" << std::endl;
		}
	}

	std::ifstream f(path, std::ios::binary);
	if (f.is_open()) {

		std::string file_str = std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());

		std::cout << "[error]{" << file_str << "}" << std::endl;

		f.close();
	}


	stream.close();

	lua["collectgarbage"]("collect");
}

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
		test_file(lua, file);
	}

	return 1;
}


void check_script(sol::state& lua, const fs::path& file, std::shared_ptr<Jass> jass) {
	ParseConfig* config = new ParseConfig();

	config->file = file.filename().string();
	
	std::ifstream stream(file, std::ios::binary);

	config->script = std::make_shared<std::string>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

	bool success = jass->run(*config);
	
	auto& errors = jass->log.errors;
	auto& warnings = jass->log.warnings;
	
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

	stream.close();
	delete config;

}

void check(sol::state& lua) {

	double mem_start = lua["collectgarbage"]("count");

	fs::path path = fs::current_path() / "war3" / "24";

	clock_t start = clock();

	auto jass = std::make_shared<Jass>();

	jass->init(lua.lua_state());

	check_script(lua, path / "common.j", jass);
	check_script(lua, path / "blizzard.j", jass);
	check_script(lua, path / "war3map.j", jass);


	std::cout << "time : " << ((double)(clock() - start) / CLOCKS_PER_SEC) << " s" << std::endl;;


	double mem_end = lua["collectgarbage"]("count");

	//check_script(lua, fs::current_path() / "tests" / "aa.j", *result);

	std::cout << "lua memory start " << mem_start / 1024 << " mb" << std::endl;
	std::cout << "lua memory end " << mem_end / 1024 << " mb" << std::endl;

	lua["collectgarbage"]("collect");
	mem_end = lua["collectgarbage"]("count");
	std::cout << "lua memory end2 " << mem_end / 1024 << " mb" << std::endl;


	return;
}


int test(lua_State* L) {

	return 0;
}


int main(int argn, char** argv) {

	std::locale::global(std::locale("zh_CN.UTF-8"));

	init_config();


	sol::state lua;

	lua_State* L = lua.lua_state();



	lua.open_libraries();

	lua.open_libraries(sol::lib::jit);

	//lua["jit"]["on"](true);
	lua["jit"]["opt"]["start"](3);

	lua.require("lpeglabel", luaopen_lpeglabel);
	
	lua.require_script("relabel", relabel_script, false, "relabel");

	lua_register(L, "timer_start", [](lua_State* L)->int {
		uint64_t begin_time;
		QueryPerformanceCounter((LARGE_INTEGER*)&begin_time);

		lua_pushinteger(L, begin_time);
		return 1;
	});
	

	auto get_time = [](uint64_t time) {
		LARGE_INTEGER litmp;
		QueryPerformanceFrequency(&litmp); //获取频率
		double time_fre = (double)litmp.QuadPart;
		double time_elapsed = (double)time / time_fre;
		return time_elapsed;
	};

	lua.require_script("peg", peg_script, false, "peg");

	//auto tbl = lua.create_table();
	//
	//lua["tbl"] = tbl;
	//
	//tbl["test"] = [](lua_State* L)->int {
	//	lua_Number node = lua_tonumber(L, 1);
	//	lua_pushnumber(L, node);
	//
	//	return 1;
	//};
	//
	//clock_t start = clock();

	//lua.script("for i = 0, 8000000 do tbl.test(i, i, i, i) end");


	//std::cout << "time : " << ((double)(clock() - start) / CLOCKS_PER_SEC) << " s" << std::endl;;

	check(lua);
	
	//delete lua;
	//lua = nullptr;
	
	sol::lua_table res = lua["timer_map"];
	
	std::cout << "Binary " << num << std::endl;
	
	std::ofstream file("out.txt", std::ios::binary);
	file << "name\ttime\tcount" << std::endl;
	for (auto v : res) {
		auto key = v.first.as<std::string>();
		double time = get_time(v.second.as<uint64_t>());
		int count = lua["count_map"][key].get<int>();

		char data[0x100];

		sprintf(data, "%s\t%.06f\t%i", key.c_str(), time, count);
		file << data << std::endl;
	}
	file.close()
;	
	lua.require_file("main", (fs::current_path() / "src2" / "main.lua").string());
	
	return 0;
}