
#include <iostream>
#include <filesystem>
#include <fstream>
#include <execution>
#include <vector>

#include <jass.hpp>

#include <utils/unicode.h>

namespace fs = std::filesystem;


bool tests(const fs::path& tests_path) {

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

	for (auto& path : paths) {
		
		tao::pegtl::file_input in(path);
		
		try {
			auto ret = tao::pegtl::parse<tao::pegtl::jass::grammar>(in);
	
			std::cout << path << ":" << ret << std::endl;
		
		} catch(const tao::pegtl::parse_error& e) {
			const auto p = e.positions().front();
			std::cerr << base::u2a(e.what()) << std::endl
				<< in.line_at(p) << std::endl
				<< std::setw(p.column) << '^' << std::endl;
			
		}
	}

}


int main(int argn, char** argv) {

	if (argn < 2) {
		return 0;
	}

	tests(fs::path(argv[1]));

	return 0;
}