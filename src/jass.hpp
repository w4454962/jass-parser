#pragma once

#include "stdafx.h"

#define ERR TAO_PEGTL_RAISE_MESSAGE
 
namespace jass {

	using namespace tao::pegtl;

	struct line_char : sor<one<'\r', '\n'>, istring<'\r', '\n'>> {};

	struct comment : if_must<two<'/'>, star<seq<not_at<line_char>, bytes<1>>>> {};

	struct whilespace : sor<one<' ', '\t'>, istring<0xef, 0xbb, 0xbf>> {};

	struct space : star<sor<comment, whilespace>> {};

	struct newline : seq<space, line_char> {};

	struct igone : star<not_at<line_char>> {};

	template< typename Key >
	struct key : seq<space, Key, not_at<identifier_other > > {};

#define KEYWORD(s)  \
	struct str_##s:TAO_PEGTL_STRING(#s) {}; \
	struct key_##s : key< str_##s > {};

	//生成关键字
#define KEYWORD_ALL	(or)(and)(not)(if)(then)(elseif)(else)(endif)(loop)(endloop)(function)(endfunction) \
	(globals)(endglobals)(native)(takes)(set)(call)(returns)(return)(exitwhen)(type)(extends)(constant) \
	(array)(local)(nothing)(debug)(true)(false)(null)

#define MACRO_DEF(r, def, s) def(s)

	BOOST_PP_SEQ_FOR_EACH(MACRO_DEF, KEYWORD, KEYWORD_ALL)



		//#define MACRO_CONCAT(r, head, s)  (BOOST_PP_CAT(head, s))
		//关键字文本合集 
		//struct sor_keyword : sor <BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(MACRO_CONCAT, str_, KEYWORD_ALL))> {};
		//关键字
		//struct keyword : key<sor_keyword> {};


	struct comma : seq<space, one<','>> {};

	struct assign : seq<space, one<'='>> {};


	//单斜杠转义符集合
	struct single : one<'a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '"', '\'', '0', '\n'> {};
	struct escaped : if_must<one<'\\'>, single> {};
	struct regular : not_one<'\r', '\n'> {};
	struct character : sor<escaped, regular> {};


	//字符串 匹配一对Q中间的内容
	template<char Q>
	struct short_string : if_must<one<Q>, until<one<Q>, character>> {};
	struct string : seq<space, short_string<'"'>> {};

	struct boolean : sor<key_true, key_false> {};

	struct digits : plus< digit> {};

	struct real : seq<space, opt<one< '-' >>, space, digits, one<'.'>, digits> {};

	struct integer256 : seq<space, short_string<'\''>> {};
	struct integer16 : seq<sor<istring<'0', 'x'>, one<'$'>>, plus<xdigit>> {};
	struct integer8 : seq<istring<'0'>, plus<odigit>> {};
	struct integer : seq<space, opt<one< '-' >>, space, sor<integer256, integer16, integer8, digits>> {};


	struct name : seq<alpha, opt<plus<sor<alnum, one<'_'>>>>>{};

	struct code_name : name {};

	struct code : seq<key_function, space, code_name> {};

	struct null : key_null {};
	struct value : sor<null, boolean, string, real, integer,  code> {};


	struct gt : istring<'>' > {};
	struct ge : istring<'>', '='> {};
	struct lt : istring<'<' > {};
	struct le : istring<'<', '='> {};
	struct eq : istring<'=', '='> {};
	struct ue : istring<'!', '='> {};

	struct add : one<'+'> {};
	struct sub : one<'-'> {};
	struct mul : one<'*'> {};
	struct div : one<'/'> {};
	struct mod : one<'%'> {};


	struct pl : seq<space, one<'('>> {};
	struct pr : seq<space, one<')'>> {};
	struct bl : seq<space, one<'['>> {};
	struct br : seq<space, one<']'>> {};

	struct exp;
	struct exp_unit;

	struct exp_neg : seq<space, sub, exp_unit> {};
	struct exp_var_name : name {};
	struct exp_var : seq<space, exp_var_name, opt<seq<bl, exp, br>>> {};
	struct exp_value : sor<value, exp_var> {};
	struct exp_call_args : seq<exp, star<seq<comma, exp>>> {};
	struct exp_call_name: name {};
	struct exp_call : seq<space, exp_call_name, pl, opt<exp_call_args>, pr> {};
	struct exp_code : seq<key_function, space, name, pl, opt<exp_call_args>, pr> {};
	struct exp_paren : seq<pl, exp, pr> {};
	struct exp_unit : sor<exp_paren, exp_code, exp_call, exp_value, exp_neg> {};

	struct exp_add_sub : sor<add, sub> {};
	struct exp_mul_div : sor<mul, div, mod> {};

	struct exp_not : key_not {};

	struct exp_compare_operator : sor< ue, eq, le, lt, ge, gt, if_must<one<'='>, ERR("ERROR_ASSIGN_AS_EQ")>> {};

	struct exp_or : key_or {};
	struct exp_and : key_and {};

	struct exp_check_mul_div : seq<exp_unit, star<seq<space, exp_mul_div, exp_unit>>> {};
	struct exp_check_add_sub : seq<exp_check_mul_div, star<seq<space, exp_add_sub, exp_check_mul_div>>> {};
	struct exp_check_not : seq<space, star<exp_not>, exp_check_add_sub> {};
	struct exp_check_compare : seq<exp_check_not, star<seq<space, exp_compare_operator, exp_check_not>>> {};
	struct exp_check_or : seq<exp_check_compare, star<seq<space, exp_or, exp_check_compare>>> {};
	struct exp_check_and : seq<exp_check_or, star<seq<space, exp_and, exp_check_or>>> {};
	struct exp : exp_check_and {};

	struct type_name: name {};
	struct type_parent_name : name {};
	struct type_extends : if_must<key_type, space, type_name, sor<key_extends, ERR("ERROR_EXTENDS_TYPE")>, space, sor<type_parent_name, ERR("ERROR_EXTENDS_TYPE")>> {};

	struct global : seq <space, not_at<key_globals, key_function, key_native>, opt<key_constant>, space, name, opt<key_array>, space, name, opt<seq<assign, exp>>> {};
	struct globals : if_must<key_globals, must<newline, star<sor<global, newline>>, key_endglobals>> {};

	struct local_name:name {};
	struct local_type:name {};
	struct local : if_must<key_local, space, local_type ,opt<key_array>, space, local_name, opt<seq<assign, exp>>, newline> {};
	struct local_list : star<sor<seq<key_constant, ERR("ERROR_CONSTANT_LOCAL")>, local, newline>> {};

	struct action;
	struct action_list : star<sor<action, newline, seq<key_local, ERR("ERROR_LOCAL_IN_FUNCTION")>>> {};

	struct action_call :seq<opt<key_debug>, key_call, exp_call> {};
	struct action_set :seq<key_set, exp_var, assign, exp> {};
	struct action_return : seq<key_return, sor<opt<exp>, space>> {};
	struct action_exitloop : seq<key_exitwhen, exp> {};

	struct if_statement : seq<key_if, exp, key_then, newline, action_list> {};
	struct elseif_statement : seq<key_elseif, exp, key_then, newline, action_list> {};
	struct else_statement : seq<key_else, newline, action_list> {};

	struct action_if : seq<if_statement, star<elseif_statement>, opt<else_statement>, key_endif> {};

	struct action_loop : if_must<key_loop, must<action_list, key_endloop>> {};

	struct action : sor<action_call, action_set, action_return, action_exitloop, action_if, action_loop> {};


	struct arg_type : name {};
	struct arg_name: name {};
	struct arg_statement : seq<space, arg_type, space, arg_name> {};
	struct args_statement : seq < sor < key_nothing,  seq<arg_statement, star<seq<comma, arg_statement>> >> > {};
	struct check_returns : sor<key_returns, if_must<key_return, ERR("ERROR_RETURN_AS_RETURNS")>> {};

	struct native_name: name{};
	struct returns_type: name {};
	struct native_statement : if_must<key_native, must<space, native_name, key_takes, args_statement, check_returns, space, returns_type>> {};

	
	struct function_name :name {};
	struct function_statement : seq<key_function, must<space, function_name, key_takes, space, args_statement, check_returns, space, returns_type, newline>>{};
	struct function_block : seq<local_list, action_list> {};

	struct function : if_must<function_statement, function_block, key_endfunction> {};

	struct chunk : sor<type_extends, globals, native_statement, function, newline> {};

	struct jass : star<chunk> {};

	struct grammar : seq<jass, eof> {};




	struct jass_node :
		parse_tree::basic_node<jass_node>
	{
		std::string_view exp_value_type;

		auto sub_string_view(size_t pos) {
			return children[pos]->string_view();
		}

		auto as_sub_input(size_t pos) {
			return children[pos]->as_memory_input();
		}
	};


	using data_type = std::string_view;

	template< typename Type >
	struct container 
	{ 
		typedef std::shared_ptr<Type> object_ptr;

		std::vector<object_ptr> list;
		std::unordered_map <data_type, object_ptr> map;

		bool save(const data_type& name, object_ptr obj) {
			if (map.find(name) != map.end()) {
				return false;
			}
			list.push_back(obj);
			map.emplace(name, obj);
			return true;
		}

		object_ptr find(const data_type& name) {
			if (map.find(name) == map.end()) {
				return nullptr;
			}
			return map.at(name);
		}

		object_ptr back() {
			return list.back();
		}
	};

	struct state_base {
		struct jass_node* node = nullptr;

		void save_node(const std::unique_ptr<struct jass_node>& n) {
			node = n.get();
		}
	};
	
	
	struct type_state : state_base {
		data_type name;
		std::shared_ptr<type_state> parent;
		data_type root;
		std::unordered_map<data_type, bool> child_map;
	};
	
	struct local_state : state_base {
		data_type type;
		data_type name;
	
		bool is_array;
		bool is_init;
	};
	
	struct global_state : state_base {
		data_type type;
		data_type name;
	
		bool is_array;
		bool is_const;
	};
	
	struct arg_state : state_base {
		data_type type;
		data_type name;
	};
	
	struct native_state : state_base
	{
		data_type name;

		container<arg_state> args;
		data_type returns_type;
	};

	struct function_state : native_state
	{
		container<local_state> locals;
	};
	

	struct jass_state
	{
		std::shared_ptr<std::string> source;

		container<type_state> types;
		container<global_state> globals;
		container<native_state> natives;
		container<function_state> functions;

		bool is_function_block = false;
		bool is_local_block = false;

		std::unordered_map<std::string_view, bool> keyword_map;


		bool is_keyword(const std::string_view& str) {
			return keyword_map.find(str) != keyword_map.end();
		}

		bool is_extends_type(const std::string_view& need_type, const std::string_view& input_type) {
			if (input_type == need_type) {
				return true;
			}
			auto ts = types.find(need_type);
			auto it = ts->child_map.find(input_type);
			if (it == ts->child_map.end()) {
				return false;
			}
			return it->second;
		}

		std::string_view get_base_type(const std::string_view& t) {
			auto type = types.find(t);
			while (type->parent) {
				type = type->parent;
			}

			return type->name;
		}


		jass_state() {
			//base type

			auto base_list = { "integer", "real", "string", "boolean", "code", "handle", "nothing"};

			for (auto name : base_list) {
				auto obj = std::make_shared<type_state>();
				obj->name = name;
				obj->root = name;
				types.save(name, obj);
				keyword_map.emplace(name, true);

				if (name == "code" || name == "string" || name == "handle") { //这些类型可以使用null
					obj->child_map.emplace("null", true);
				} else if (name == "real"){ // real类型 可以使用integer
					obj->child_map.emplace("integer", true);
				}
			}


#define MACRO_INPUT(s) keyword_map.emplace(#s, true);
	
			//关键字文本合集 
			BOOST_PP_SEQ_FOR_EACH(MACRO_DEF, MACRO_INPUT, KEYWORD_ALL)
	
		}

		void set_source(const std::string& s) {
			source = std::make_shared<std::string>(s);
		}

	};


	inline std::string_view get_match_exp_type(const std::string_view& t1, const std::string_view& t2) {
		if (t1 == demangle<integer>()) {
			if (t2 == demangle<integer>()) {
				return demangle<integer>();
			}
			else if (t2 == demangle<real>()) {
				return demangle<real>();
			}
		}
		else if (t1 == demangle<real>()) {
			if (t2 == demangle<integer>() || t2 == demangle<real>()) {
				return demangle<real>();
			}
		}
		return "";
	}

	inline std::string_view get_string_connect(const std::string_view& t1, const std::string_view& t2) {
		if ((t1 == demangle<string>() || t1 == demangle<null>()) && (t2 == demangle<string>() || t2 == demangle<null>())) {
			return demangle<string>();
		}
		return "";
	}

	template<typename... Args>
	inline std::string error_format(std::string_view msg, Args... args) {
		return std::format(convert_message(msg), args...);
	}

	class jass_parse_error :public parse_error
	{
	public:
		template<typename ParseInput, typename... Args>
		jass_parse_error(const ParseInput& in, const std::string_view& msg, Args... args)
			: parse_error(error_format(msg, args...), in)
		{ }

	};


	
	// some nodes don't need to store their content
	struct type_extends_content
		: parse_tree::apply< type_extends_content >
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			auto name = n->sub_string_view(0);
			if (s.is_keyword(name)){//名字是关键字
				throw jass_parse_error(n->as_sub_input(0), "ERROR_KEY_WORD", name);
			}

			auto ptr = s.types.find(name);
			if (ptr) { //重复定义类型
				auto input = n->as_sub_input(0);
				auto pos = ptr->node->begin();
				throw jass_parse_error(input, "ERROR_REDEFINE_TYPE", name, pos.source, pos.line);
			}

			auto type = n->sub_string_view(1);
			auto parent = s.types.find(type);
			if (!parent) { //类型未定义
				throw jass_parse_error(n->as_sub_input(1), "ERROR_UNDEFINE_TYPE", type);
			}
			auto ts = std::make_shared<type_state>();
			ts->name = name;
			ts->parent = parent;
			ts->save_node(n);
			ts->root = parent->root;

			//继承置空的能力 
			if (auto it = parent->child_map.find("null"); it->second) {
				ts->child_map.emplace("null", true);
			}

			s.types.save(name, ts);
			s.keyword_map.emplace(name, true);

			while (parent){
				parent->child_map.emplace(name, true);
				parent = parent->parent;
			}
		}
	};


	//检查native function 的函数申明格式
	struct function_statement_content
		: parse_tree::apply< function_statement_content >
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			auto name = n->sub_string_view(0);
			if (s.is_keyword(name)) {//名字是关键字
				throw jass_parse_error(n->as_sub_input(0), "ERROR_KEY_WORD", name);
			}

			std::vector<void*> check_list = { &s.natives, &s.functions, &s.globals };

			for (auto table : check_list) {
				auto ptr = ((container<state_base>*)table)->find(name);
				if (ptr) { //重复定义
					auto input = n->as_sub_input(0);
					auto pos = ptr->node->begin();
					throw jass_parse_error(input, "ERROR_REDEFINE_FUNCTION", name, pos.source, pos.line);
				}
			}

			std::shared_ptr<native_state> ns;

			if (n->is_type<native_statement>()) {
				ns = std::make_shared<native_state>();
				s.natives.save(name, ns);
			}
			else if (n->is_type<function_statement>()) {
				auto fs = std::make_shared<function_state>();
				ns = fs;
				s.functions.save(name, fs);
				s.is_function_block = true;
			}

			ns->name = name;
			ns->save_node(n);

			auto& args = n->children[1];
			//参数数量以及类型
			if (args->has_content()) {
				for (auto& arg : args->children) {
					auto arg_type = arg->sub_string_view(0);
					auto arg_name = arg->sub_string_view(1);

					if (!s.types.find(arg_type)) { //参数类型未定义
						throw jass_parse_error(arg->as_sub_input(0), "ERROR_UNDEFINE_TYPE", arg_type);
					}

					if (s.is_keyword(arg_name)) {//参数名字是关键字
						throw jass_parse_error(arg->as_sub_input(1), "ERROR_KEY_WORD", arg_name);
					}

					auto ptr = ns->args.find(arg_name);
					if (ptr) { //参数名字重复定义
						auto input = arg->as_sub_input(1);
						auto pos = ptr->node->begin();
						throw jass_parse_error(input, "ERROR_REDEFINE_ARG", arg_name, pos.source, pos.line);
					}

					auto as = std::make_shared<arg_state>();
					as->name = arg_name;
					as->type = arg_type;
					as->save_node(n);
					ns->args.save(arg_name, as);
				}
			}

			auto return_type = n->sub_string_view(2);
			if (!s.types.find(return_type)) { //返回类型未定义
				throw jass_parse_error(n->as_sub_input(2), "ERROR_UNDEFINE_TYPE", return_type);
			}

			ns->returns_type = return_type;
		}
	};

	

	//检查local局部变量申明格式
	struct local_content
		: parse_tree::apply< local_content >
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			auto fs = s.functions.back();

			auto ls = std::make_shared<local_state>();
			ls->is_array = false;
			ls->is_init = false;
			ls->save_node(n);
			
			auto size = n->children.size();

			auto check_name = [&](size_t pos) {
				auto& node = n->children[pos];
				auto name = node->string_view();
				if (s.is_keyword(name)) {//局部变量名字是关键字
					throw jass_parse_error(node->as_memory_input(), "ERROR_KEY_WORD", name);
				}

				auto ptr = fs->args.find(name);
				if (ptr) { //局部变量跟参数重名
					position p = ptr->node->begin();
					if (ls->is_array) {
						throw jass_parse_error(node->as_memory_input(), "ERROR_REDEFINE_ARRAY_WITH_ARG", name, fs->name, p.source, p.line);
					}
					throw jass_parse_error(node->as_memory_input(), "ERROR_REDEFINE_VAR_TYPE_WITH_ARG", name, ls->type, fs->name, ptr->type, p.source, p.line);
				}

				auto lp = fs->locals.find(name);
				if (lp) { //局部变量重复定义
					position p = lp->node->begin();
					throw jass_parse_error(node->as_memory_input(), "WARNING_REDEFINE_VAR", name, p.source, p.line);
				}

				auto fp = s.functions.find(name);
				if (fp) { //局部变量跟函数名重复
					position p = fp->node->begin();
					throw jass_parse_error(node->as_memory_input(), "ERROR_REDEFINE_FUNCTION", name, p.source, p.line);
				}

				auto np = s.natives.find(name);
				if (np) { //局部变量跟native重复
					position p = np->node->begin();
					throw jass_parse_error(node->as_memory_input(), "ERROR_DEFINE_NATIVE_TYPE", name, p.source, p.line);
				}

				return name;
			};

			auto check_type = [&](size_t pos) {
				auto& node = n->children[pos];
				auto type = node->string_view();
				if (!s.types.find(type)) {//局部变量类型不存在
					throw jass_parse_error(node->as_memory_input(), "ERROR_UNDEFINE_TYPE", type);
				}
				return type;
			};

			jass_node* exp = nullptr;
			if (size == 2) {
				ls->type = check_type(0);
				ls->name = check_name(1);

			} else if (size == 3) {
				ls->type = check_type(0);

				if (n->children[1]->is_type<key_array>()) { 

					ls->is_array = true;
					ls->name = check_name(2);

					if (ls->type == demangle<code>()) { //code类型不能是数组
						throw jass_parse_error(n->as_sub_input(0), "ERROR_CODE_ARRAY");
						
					}
				} else {
					ls->name = check_name(1);
					auto& exp = n->children[2];

					if (!s.is_extends_type(ls->type, exp->exp_value_type)) { //值类型 跟变量类型不符
						throw jass_parse_error(n->as_sub_input(2), "ERROR_SET_TYPE", ls->name, ls->type, exp->exp_value_type);
					}

					ls->is_init = true;
				}
			} else if (size == 4) { //数组不能直接初始化
				throw jass_parse_error(n->as_sub_input(3), "ERROR_ARRAY_INIT");
			}
			fs->locals.save(ls->name, ls);
		}
	};

	struct exp_content
		: parse_tree::apply< exp_content >
	{
		



		static std::string_view check_exp_type(std::unique_ptr<jass_node>& first, std::unique_ptr<jass_node>& op, std::unique_ptr<jass_node>& second, jass_state& s) {
			std::string str = op->string();
			uint32_t byte = *(uint32_t*)(str.c_str());

			std::string_view t1 = first->exp_value_type, t2 = second->exp_value_type;
			std::string_view exp_type = "nothing";

			switch (byte)
			{
			case '+':
				exp_type = get_match_exp_type(t1, t2); //如果是数字相加
 
				if (exp_type.empty()) { //否则 如果是 字符串连接
					exp_type = get_string_connect(t1, t2);
				}
				if (exp_type.empty()) {
					throw jass_parse_error(op->as_memory_input(), "ERROR_ADD", t1, t2);
				}
				break;
			case '-':
				exp_type = get_match_exp_type(t1, t2); 
				if (exp_type.empty()) {
					throw jass_parse_error(op->as_memory_input(), "ERROR_SUB", t1, t2);
				}
				break;

			case '*':
				exp_type = get_match_exp_type(t1, t2);
				if (exp_type.empty()) {
					throw jass_parse_error(op->as_memory_input(), "ERROR_MUL", t1, t2);
				}
				break;

			case '/':
				exp_type = get_match_exp_type(t1, t2);
				if (exp_type.empty()) {
					throw jass_parse_error(op->as_memory_input(), "ERROR_DIV", t1, t2);
				}
				break;

			case '%':
			
				if (t1 == demangle<integer>() && t2 == demangle<integer>()) {
					exp_type = t1;
				} else {
					throw jass_parse_error(op->as_memory_input(), "ERROR_MOD");
				}
				break;

			case '==':
			case '!=':
				if (t1 == demangle<null>() && t2 == demangle<null>()) {
					exp_type = demangle<boolean>();
				} else if (!get_match_exp_type(t1, t2).empty()) {
					exp_type = demangle<boolean>();
				} else if (s.get_base_type(t1) == s.get_base_type(t2)) {
					exp_type = demangle<boolean>();
				} else {
					throw jass_parse_error(op->as_memory_input(), "ERROR_EQUAL", t1, t2);
				}
				break;

			case '>':
			case '<':
			case '>=':
			case '<=':
				if (!get_match_exp_type(t1, t2).empty()) {
					exp_type = demangle<boolean>();
				} else {
					throw jass_parse_error(op->as_memory_input(), "ERROR_COMPARE", t1, t2);
				}
				break;
			case 'and':
			case 'or':

				if (t1 == demangle<boolean>() && t2 == demangle<boolean>()) {
					exp_type = demangle<boolean>();
				} else {
					throw jass_parse_error(op->as_memory_input(), "ERROR_AND", t1, t2);
				}
				break;
			default:
				throw jass_parse_error(op->as_memory_input(), "UNKNOWN_EXP");
				break;
			}
			return exp_type;
		}

		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			if (n->children.size() == 1) { //没有操作符的匹配
				n = std::move(n->children.front());
				return;
			}

			for (size_t i = 1; i < n->children.size(); i += 2) {
				auto& first = n->children[i - 1];
				auto& op = n->children[i];
				auto& second = n->children[i + 1];
	
				n->exp_value_type = check_exp_type(first, op, second, s);
			}
			
			//std::cout << "type " << n->type << " " << n->string_view() << " size " << n->children.size()  << " " << n->exp_value_type << std::endl;
	
		}
	};


	struct value_content
		: parse_tree::apply< value_content >
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			n->exp_value_type = n->type;
		}
	};

	struct var_content
		: parse_tree::apply<var_content>
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			auto name = n->sub_string_view(0);

			bool has_index = n->children.size() > 1;

			if (s.is_function_block) { //如果是在函数里引用变量值 优先局部变量
				auto fs = s.functions.back();

				if (auto as = fs->args.find(name); as) {
					if (has_index) { //参数不需要索引
						throw jass_parse_error(n->as_sub_input(1), "ERROR_WASTE_INDEX", name);
					}
					n->exp_value_type = as->type;
				} else if (auto ls = fs->locals.find(name); ls) {
					if (has_index && !ls->is_array) { //局部变量不需要索引
						throw jass_parse_error(n->as_sub_input(1), "ERROR_WASTE_INDEX", name);
					}else if (!has_index && ls->is_array) { //局部数组缺少索引
						throw jass_parse_error(n->as_sub_input(0), "ERROR_NO_INDEX", name);
					}else if (has_index && ls->is_array) {
						auto& exp = n->children[1];
						if (exp->exp_value_type != demangle<integer>()) { //索引类型错误
							throw jass_parse_error(n->as_sub_input(1), "ERROR_INDEX_TYPE", name, exp->exp_value_type);
						}else if (s.is_local_block)  { //在局部变量申明区使用未初始化的局部数组 
							throw jass_parse_error(n->as_sub_input(0), "ERROR_GET_UNINIT", name);
						}
					}

					if (!ls->is_array && !ls->is_init) { //非数组的局部变量 如果没有初始化就引用 进行报错
						throw jass_parse_error(n->as_sub_input(0), "ERROR_GET_UNINIT", name);
					}

					n->exp_value_type = ls->type;
				} else if (auto gs = s.globals.find(name); gs) {
					if (has_index && !gs->is_array) { //全局变量不需要索引
						throw jass_parse_error(n->as_sub_input(0), "ERROR_WASTE_INDEX", name);
					} else if (!has_index && gs->is_array) { //全局数组缺少索引
						throw jass_parse_error(n->as_sub_input(1), "ERROR_NO_INDEX", name);
					} else if (has_index && gs->is_array) {
						auto& exp = n->children[1];
						if (exp->exp_value_type != demangle<integer>()) { //索引类型错误
							throw jass_parse_error(n->as_sub_input(1), "ERROR_INDEX_TYPE", name, exp->exp_value_type);
						}
					}
					n->exp_value_type = ls->type;
				} else {
					//变量不存在
					throw jass_parse_error(n->as_sub_input(0), "VAR_NO_EXISTS", name);
				}
				
			}
		}
	};
	
	
	struct exp_call_content
		: parse_tree::apply<exp_call_content>
	{


		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			auto name = n->sub_string_view(0);
			bool has_params = n->children.size() > 1;

			std::shared_ptr<native_state> f;
			 if(auto ns = s.natives.find(name); ns) { //调用本地函数
				f = ns;
			} else if (auto fs = s.functions.find(name); fs) { //调用自定义函数
				f = fs;
			} else {
				//函数不存在
				throw jass_parse_error(n->as_sub_input(0), "FUNCTION_NO_EXISTS", name);
			}
			
			size_t param_count = 0;
			size_t args_count = f->args.list.size();

			if (has_params) {
				param_count = n->children[1]->children.size();
			}

			if (s.is_local_block) { 
				if (name == s.functions.back()->name) { //在局部变量申明区进行递归
					throw jass_parse_error(n->as_sub_input(0), "ERROR_LOCAL_RECURSION");
				}
			}
			
			if (param_count > args_count) {//参数填多了
				throw jass_parse_error(n->as_sub_input(0), "ERROR_MORE_ARGS", name, args_count, param_count);
			} else if (param_count < args_count){
				//参数数量少了
				std::stringstream ss;
				for (size_t i = param_count; i < args_count; i++) {
					auto& arg = f->args.list[i];
					if (i > param_count)
						ss << ", ";
					ss << arg->type << " " << arg->name;
				}
				throw jass_parse_error(n->as_sub_input(0), "ERROR_LESS_ARGS", name, args_count, param_count, ss.str());
			} else if (has_params){
				//参数数量一致的情况下 检查参数类型
				auto& params = n->children[1];

				for (size_t i = 0; i < args_count; i++) {
					auto& param = params->children[i];
					auto& arg = f->args.list[i];

					if (!s.is_extends_type(arg->type, param->exp_value_type)) {
						throw jass_parse_error(param->as_memory_input(), "ERROR_WRONG_ARG", name, i + 1, arg->type, param->exp_value_type) ;
					}
				}
			}

			n->exp_value_type = f->returns_type;

		}
	};

	template<typename Rule>
	using selector = parse_tree::selector<
		Rule,

		type_extends_content::on<type_extends>,
		parse_tree::store_content::on<
			type_name,
			type_parent_name
		>,

		function_statement_content::on<native_statement, function_statement>,
		parse_tree::store_content::on<
			function_name,
			native_name,
			returns_type,

			args_statement,
				arg_statement,
					arg_name,
					arg_type
		>,

		local_content::on<local>,
		parse_tree::store_content::on<
			local_type,
			local_name,
			key_array
		>,


		exp_content::on<
			exp_check_and,
			exp_check_or,
			exp_check_compare,
			exp_check_add_sub,
			exp_check_mul_div
		>,
	
		parse_tree::store_content::on<
			add, sub, mul, div, mod, gt, ge, lt, le, eq, ue
		>,

		value_content::on<null, boolean, string, real, integer, code>,
		var_content::on<exp_var>,
		exp_call_content::on<exp_call>,

		parse_tree::store_content::on<
			exp_var_name,
			exp_call_name,
			exp_call_args
		>
	>;

	
	template< typename Rule >
	struct check_action 
		:nothing<Rule>
	{};

	template<>
	struct check_action<key_endfunction> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {
			s.is_function_block = false;
		}
	};
	template<>
	struct check_action<function_statement> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {
			s.is_local_block = true;
		}
	};
	template<>
	struct check_action<local_list> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {
			s.is_local_block = false;
		}
	};


	//自定义规则获取一行
	template<typename ParseInput>
	std::string_view line_at(ParseInput& in, const tao::pegtl::position& p)
	{
		const char* b = in.begin_of_line(p);

		using input_t = memory_input< tracking_mode::lazy, eol::lf_crlf, const char* >;
		input_t in2(in.at(p), in.end(), "");
		using line = until<line_char, bytes<1>>;

		(void)normal< line >::match< apply_mode::nothing, rewind_mode::dontcare, nothing, normal >(in2);

		return std::string_view(b, static_cast<std::size_t>(in2.current() - b));
	}

}


#undef ERR

#undef MACRO_DEF
#undef KEYWORD_ALL
#undef KEYWORD
#undef MACRO_INPUT