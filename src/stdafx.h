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

#pragma warning(disable:4309)

std::string_view convert_message(std::string_view msg);
