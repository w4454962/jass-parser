#pragma once

#pragma execution_character_set("UTF-8")  


#include <iostream>
#include <filesystem>
#include <fstream>
#include <execution>
#include <vector>
#include <utils/unicode.h>
#include <unordered_map>
#include <map>
#include <regex>
#include <memory>
#include <string>
#include <string_view>
#include <sstream>
#include <stack>
#include <array>
#include <set>
#define __cpp_lib_format
#include <format>


//#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#pragma warning(disable:4309 4455)

namespace fs = std::filesystem;
 


std::string_view convert_message(std::string_view msgt);


typedef std::uint32_t hash_t;

constexpr hash_t prime = 0x000001B3u;
constexpr hash_t basis = 0x84222325u;

hash_t hash_s(const std::string_view& str);


constexpr hash_t hash_compile_time(char const* str, hash_t last_value = basis)
{
	return *str ? hash_compile_time(str + 1, (*str ^ last_value) * prime) : last_value;
}

constexpr unsigned int operator "" s_hash(char const* p, size_t)
{
	return hash_compile_time(p);
}

template< typename T >
[[nodiscard]] constexpr std::string_view demangle() noexcept
{
	// we can not add static_assert for additional safety,
	// see issues #296, #301 and #308
	constexpr std::string_view sv = __FUNCSIG__;
	constexpr auto begin = sv.find("demangle<");
	constexpr auto tmp = sv.substr(begin + 9 );
	constexpr auto end = tmp.rfind('>');
	return tmp.substr(0, end);
}
