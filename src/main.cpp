


#include "jass.hpp"
#include <tao/pegtl/contrib/coverage.hpp>
#include <tao/pegtl/contrib/print_coverage.hpp>

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


	err = std::regex_replace(err, std::regex("E:\\\\jass\\-parser\\\\tests\\\\should\\-fail\\\\"), "");

	

	std::vector<char> of(p.column -1, ' ');
	std::vector<char> tip(width, '^');

	std::string code = std::format("{}\n{}{}", line, std::string(of.begin(), of.end()), std::string(tip.begin(), tip.end()));
	std::string error = jass::error_format("ERROR_POS", err, p.source, p.line, code);

	std::cout << error << std::endl;

	std::string src = std::string(p.source.begin(), p.source.end());

	fs::path path = std::regex_replace(src, std::regex("\\.j"), ".err");

	if (!fs::exists(path)) {
		path = std::regex_replace(src, std::regex("\\.j"), ".warn");
	}

	std::cout << path << "  error " << std::endl;

	if (fs::exists(path)) {
		std::ifstream file(path, std::ios::binary);
		std::stringstream ss;

		if (file.is_open()) {
			ss << file.rdbuf();

			std::string file_str = ss.str();
			file_str = std::regex_replace(file_str, std::regex("\\(.+\\)"), "");
			//if (file_str != err) {
			//	std::cout << error << std::endl;

				std::cout 
					<< "<" << err << ">" << std::endl
					<< "{" << file_str << "}" << std::endl;
			//}

			file.close();
		}
	}

}





//bool tests(const fs::path& tests_path) {
//
//	std::cout << "each path " << tests_path << std::endl;
//
//	if (!fs::exists(tests_path)) {
//		return 0;
//	}
//
//	std::vector<fs::path> paths;
//
//	for (const auto i : fs::recursive_directory_iterator(tests_path)) {
//		if (i.is_regular_file() && (i.path().extension() == ".j" || i.path().extension() == ".J")) {
//			paths.push_back(i.path());
//		}
//	}
//
//	//paths = { fs::path(tests_path / "should-fail" / "逻辑错误-1.j") };
//	 //paths = { fs::path(tests_path / "aa.j") };
//
//	int i = 0;
//
//	for (auto& path : paths) {
//		
//		tao::pegtl::file_input in(path);
//
//		
//		try {
//			std::string str;
//
//			jass::jass_state state;
//	
//			const auto root = parse_tree::parse<jass::grammar, jass::jass_node, jass::selector, jass::check_action>(in, state);
//			std::cout << path <<  "  pass" << std::endl;
//
//		} catch (const jass::jass_parse_error& e) {
//			const auto p = e.positions().front();
//
//			
//			output_error(p, e.width, jass::line_at(in, p), e.message());
//
//		} catch(const tao::pegtl::parse_error& e) {
//			const auto p = e.positions().front();
//
//			output_error(p, 1, jass::line_at(in, p), e.message());
//
//		}
//		
//	}
//
//	return 1;
//}


void init_config() {
	
	fs::path path = fs::current_path() / "locale" / "zh-CN" / "parser.lng";

	tao::pegtl::file_input in(path);

	try {
		std::string line, key;
		while (1) {
			std::string_view line_view = jass::line_at(in, in.position());
			line = std::string(line_view.begin(), line_view.end() - 1);
			std::smatch result;
			if (std::regex_match(line, result, std::regex("^\\[(.+)\\]$"))) {
				key = result[1];
			} else if (!key.empty()) {
				std::string& old = jass_error_map[key];
				if (old.empty()) {
					old += line;
				} else {
					old += "\n" + line;
				}
			}
			if (line_view.data() + line_view.size() == in.end()) {
				break;
			}
			in.bump_to_next_line(line_view.size());
		}
	} catch(...) {
	
	}

	
	for (auto&& [k, v] : jass_error_map) {
		//将旧版本的格式化代码 修改成新版本的
		
		v = std::regex_replace(v, std::regex("%(d)"), "{:$1}");
		v = std::regex_replace(v, std::regex("%s"), "{}");
	} 
	 
	//for (auto&& [k, v] : jass_error_map) {
	//	std::cout << k << ":" << v << std::endl;
	//}
}

//#define DEBUG_COUNT

#ifdef DEBUG_COUNT
	coverage_result check_action_result;
#endif

std::unique_ptr<jass::jass_node> check_script( file_input<>& in, jass::jass_state& state)
{
		
	try {

		state.temp.has_function = false;

		auto root = parse_tree2::parse<jass::grammar, jass::jass_node, jass::selector, jass::check_action>(in, state);
		std::cout << in.source() <<  "  pass" << std::endl;

#ifdef DEBUG_COUNT
		in.restart();
		const bool success = coverage<jass::grammar>(in, check_action_result);
#endif
		

		return std::move(root);

	} catch (const jass::jass_parse_error& e) {
		const auto p = e.positions().front();

		
		output_error(p, e.width, jass::line_at(in, p), e.message());

	} catch(const tao::pegtl::parse_error& e) {
		const auto p = e.positions().front();

		output_error(p, 1, jass::line_at(in, p), e.message());
	}
		
	return nullptr;
}


void check() {
	fs::path path = fs::current_path() / "war3" / "24";

	jass::jass_state state;

	clock_t start = clock();

	file_input common(path / "common.j");
	file_input blizzard(path / "blizzard.j");
	file_input war3map(path / "war3map.j");
	
	
	check_script(common, state);
	check_script(blizzard, state);
	
	 
	auto war3map_ast = check_script(war3map, state);

	std::cout << "time : " << ((double)(clock() - start) / CLOCKS_PER_SEC) << " s" << std::endl;;

#ifdef DEBUG_COUNT
	std::ofstream file("out.txt", std::ios::binary);

	file << check_action_result;
	std::cout << check_action_result << std::endl;
	file.close();
	
	std::ofstream file2("out2.txt", std::ios::binary);
	file2 << "name\tstart\tsuccess\tfailure\traise\n";
	
	for (auto&& [k, v] : check_action_result) {
		file2 << k << "\t" << v.start << "\t" << v.success << "\t" << v.failure << "\t" << v.raise << "\n";
	}
	file2.close();
#endif 

}
int main(int argn, char** argv) {

	std::locale::global(std::locale("zh_CN.UTF-8"));

	if (argn < 2) {
		return 0;
	}
	
	init_config();

	check();

	//tests(fs::path(argv[1]));

	return 0;
}