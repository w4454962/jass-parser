#pragma once

#include <iostream>
#include <filesystem>
#include <fstream>
#include <execution>
#include <vector>
#include <utils/unicode.h>
#include <unordered_map>

#include <tao/pegtl.hpp>
#include <tao/pegtl/parse_error.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>


#include <boost/preprocessor.hpp>

#undef or 
#undef and
#undef not

#pragma warning(disable:4309)
