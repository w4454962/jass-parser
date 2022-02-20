


#include "jass.hpp"

namespace fs = std::filesystem;


std::unordered_map<std::string, std::string> jass_error_map;

using namespace  tao::pegtl;


std::string_view convert_message(std::string_view msg) {
	if (jass_error_map.find(msg.data()) != jass_error_map.end()) {
		return jass_error_map.at(msg.data());
	}
	return msg;
}


template<typename... Args>
void output_error(const position& p, size_t width, std::string_view line, std::string_view msg, Args... args) {

	std::string err = jass::error_format(msg, args...);

	std::vector<char> of(p.column -1, ' ');
	std::vector<char> tip(width, '^');

	std::string code = std::format("{}\n{}{}", line, std::string(of.begin(), of.end()), std::string(tip.begin(), tip.end()));
	std::string error = jass::error_format("ERROR_POS", err, p.source, p.line, code);

	std::cout << error << std::endl;
}





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

	//paths.push_back(tests_path /"should-fail" / "code不能有参数.j");
	//paths.push_back(tests_path / "aa.j");
	
	int i = 0;

	for (auto& path : paths) {
		
		tao::pegtl::file_input in(path);

		
		try {
			std::string str;

			jass::jass_state state;
			
			std::cout << path << std::endl;

			const auto root = parse_tree::parse<jass::grammar, jass::jass_node, jass::selector, jass::check_action>(in, state);
		
		}
		catch (const jass::jass_parse_error& e) {
			const auto p = e.positions().front();

			
			output_error(p, e.width, jass::line_at(in, p), e.message());

			//i++;
			//if (i > 1) {
			//	break;
			//}
		} catch(const tao::pegtl::parse_error& e) {
			const auto p = e.positions().front();

			output_error(p, 1, jass::line_at(in, p), e.message());

			//i++;
			//if (i > 1) {
			//	break;
			//}
			
		}
		
	}

	return 1;
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

int main(int argn, char** argv) {

	std::locale::global(std::locale("zh_CN.UTF-8"));

	if (argn < 2) {
		return 0;
	}
	
	init_config();

	tests(fs::path(argv[1]));

	return 0;
}