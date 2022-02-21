#pragma once

#pragma execution_character_set("UTF-8")  


#include <iostream>
#include <filesystem>
#include <fstream>
#include <execution>
#include <vector>
#include <utils/unicode.h>
#include <unordered_map>
#include <regex>
#include <memory>
#include <string>
#include <sstream>
#include <stack>
#define __cpp_lib_format

#include <format>



#include <tao/pegtl.hpp>
#include <tao/pegtl/parse_error.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>


#include <boost/preprocessor.hpp>

#undef or 
#undef and
#undef not

#pragma warning(disable:4309 4455)

std::string_view convert_message(std::string_view msg);



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