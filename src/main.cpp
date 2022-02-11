


#include <jass.hpp>
#include <lng.hpp>

namespace fs = std::filesystem;

std::unordered_map<std::string, std::string> jass_error_map;

using namespace  tao::pegtl;

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
				<< std::setw(p.column) << '^' << std::endl
				<< e.message() << std::endl;
			
			break;
		}
	}

	return 1;
}


void init_config() {
	
	fs::path path = fs::current_path() / "locale" / "zh-CN" / "parser.lng";

	if (!fs::exists(path)) {
		return;
	}

	tao::pegtl::file_input in(path);

	try {

		const auto root = tao::pegtl::parse_tree::parse<lng::grammar,lng::selector, nothing, lng::control >(in);
		//parse_tree::print_dot(std::cout, *root);

		for (auto& node : root->children) {
			if (node->is_type<lng::item>() && node->has_content() && node->children.size() >= 2) {
				auto key = node->children[0]->string_view();
				auto value = node->children[1]->string_view();

				jass_error_map.emplace(key, value);
				//std::cout << key << ":" << base::u2a(value) << std::endl;
			}
			
		}
	}
	catch (const tao::pegtl::parse_error& e) {
		const auto p = e.positions().front();
		std::cerr << base::u2a(e.what()) << std::endl
			<< in.line_at(p) << std::endl
			<< std::setw(p.column) << '^' << std::endl;
	}

}

int main(int argn, char** argv) {

	if (argn < 2) {
		return 0;
	}
	
	init_config();

	//tests(fs::path(argv[1]));

	return 0;
}