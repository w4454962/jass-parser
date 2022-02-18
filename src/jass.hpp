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
	struct frac : seq<one<'.'>, digits > {};
	struct real : seq<space, opt<one< '-' >>, space, digits, opt<frac>> {};

	struct integer256 : seq<space, short_string<'\''>> {};
	struct integer16 : if_must<sor<istring<'0', 'x'>, one<'$'>>, plus<xdigit>> {};
	struct integer8 : if_must<istring<'0'>, plus<odigit>> {};
	struct integer : seq<space, opt<one< '-' >>, space, sor<integer256, integer16, integer8, digits>> {};

	struct name : seq<space, seq<alpha, opt<plus<sor<alnum, one<'_'>>>>>> {};

	struct type: seq<alpha, opt<plus<sor<alnum, one<'_'>>>>>{};

	struct code_name : name {};

	struct code : seq<key_function, space, code_name> {};

	struct value : sor<key_null, boolean, string, real, integer, code> {};


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
	struct exp_var : seq<name, opt<seq<bl, exp, br>>> {};
	struct exp_value : sor<value, exp_var> {};
	struct exp_call_args : seq<exp, star<seq<comma, exp>>> {};
	struct exp_call : seq<name, pl, opt<exp_call_args>, pr> {};
	struct exp_code : seq<key_function, name, pl, opt<exp_call_args>, pr> {};
	struct exp_paren : seq<pl, exp, pr> {};
	struct exp_unit : sor<exp_paren, exp_code, exp_call, exp_value, exp_neg> {};

	struct exp_add_sub : sor<add, sub> {};
	struct exp_mul_div : sor<mul, div> {};

	struct exp_not : key_not {};

	struct exp_compare_operator : sor< ue, eq, le, lt, ge, gt, if_must<one<'='>, ERR("ERROR_ASSIGN_AS_EQ")>> {};

	struct exp_or : key_or {};
	struct exp_and : key_and {};

	struct exp_check_mul : seq<exp_unit, star<seq<space, exp_mul_div, exp_unit>>> {};
	struct exp_check_add : seq<exp_check_mul, star<seq<space, exp_add_sub, exp_check_mul>>> {};
	struct exp_check_not : seq<space, star<exp_not>, exp_check_add> {};
	struct exp_check_compare : seq<exp_check_not, star<seq<space, exp_compare_operator, exp_check_not>>> {};
	struct exp_check_or : seq<exp_check_compare, star<seq<space, exp_or, exp_check_compare>>> {};
	struct exp_check_and : seq<exp_check_or, star<seq<space, exp_and, exp_check_or>>> {};
	struct exp : exp_check_and {};

	struct type_name: name {};
	struct type_parent_name : type {};
	struct type_extends : if_must<key_type, space, type_name, sor<key_extends, ERR("ERROR_EXTENDS_TYPE")>, space, sor<type_parent_name, ERR("ERROR_EXTENDS_TYPE")>> {};

	struct global : seq <space, not_at<key_globals, key_function, key_native>, opt<key_constant>, name, opt<key_array>, name, opt<seq<assign, exp>>> {};
	struct globals : if_must<key_globals, must<newline, star<sor<global, newline>>, key_endglobals>> {};

	struct local_name:name {};
	struct local_type:type {};
	struct local : if_must<key_local, space, local_type ,opt<key_array>, space, local_name, opt<seq<assign, exp>>> {};
	struct local_list : star<sor<local, newline>> {};

	struct action;
	struct action_list : star<sor<action, newline, seq<local, ERR("ERROR_LOCAL_IN_FUNCTION")>>> {};

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


	struct arg_type : type {};
	struct arg_name: name {};
	struct arg_statement : seq<space, arg_type, space, arg_name> {};
	struct args_statement : seq < sor < key_nothing,  seq<arg_statement, star<seq<comma, arg_statement>> >> > {};
	struct check_returns : sor<key_returns, if_must<key_return, ERR("ERROR_RETURN_AS_RETURNS")>> {};

	struct native_name: name{};
	struct returns_type: type{};
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
	
	struct exp_state : state_base {
	
	};
	
	struct type_state : state_base {
		data_type name;
		std::shared_ptr<type_state> parent;
	};
	
	struct local_state : state_base {
		data_type type;
		data_type name;
	
		bool is_array;
	};
	
	struct global_state : state_base {
		data_type type;
		data_type name;
		std::shared_ptr<exp_state> value;
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

		std::unordered_map<std::string_view, bool> keyword_map;


		bool is_keyword(const std::string_view& str) {
			return keyword_map.find(str) != keyword_map.end();
		}

		jass_state() {
			//base type

			auto base_list = { "integer", "real", "string", "boolean", "code", "handle", "nothing"};

			for (auto name : base_list) {
				auto obj = std::make_shared<type_state>();
				obj->name = name;
				types.save(name, obj);
				keyword_map.emplace(name, true);
			}


#define MACRO_INPUT(s) keyword_map.emplace(#s, true);
	
			//关键字文本合集 
			BOOST_PP_SEQ_FOR_EACH(MACRO_DEF, MACRO_INPUT, KEYWORD_ALL)
	
		}

		void set_source(const std::string& s) {
			source = std::make_shared<std::string>(s);
		}

	};




	template<typename... Args>
	inline std::string error_format(std::string_view msg, Args... args) {
		return std::format(convert_message(msg), args...);
	}

	class jass_parse_error :public parse_error
	{
	public:
		template<typename ParseInput, typename... Args>
		jass_parse_error(const ParseInput& in, const std::string& msg, Args... args)
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
			s.types.save(name, ts);
			s.keyword_map.emplace(name, true);
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
					
				} else {
					ls->name = check_name(1);
					exp = n->children[2].get();
				}
			}
			
			fs->locals.save(ls->name, ls);

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
			key_array,
			exp
		>
	
	>;

	
}


#undef ERR

#undef MACRO_DEF
#undef KEYWORD_ALL
#undef KEYWORD
#undef MACRO_INPUT