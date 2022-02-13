#pragma once

#include "stdafx.h"

#define ERR TAO_PEGTL_RAISE_MESSAGE
 
namespace tao::pegtl::jass {

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

#undef MACRO_DEF
#undef KEYWORD_ALL
#undef KEYWORD


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
	struct type_parent_name : name {};
	struct type_extends : if_must<key_type, space, type_name, sor<key_extends, ERR("ERROR_EXTENDS_TYPE")>, space, sor<type_parent_name, ERR("ERROR_EXTENDS_TYPE")>> {};

	struct global : seq <space, not_at<key_globals, key_function, key_native>, opt<key_constant>, name, opt<key_array>, name, opt<seq<assign, exp>>> {};
	struct globals : if_must<key_globals, must<newline, star<sor<global, newline>>, key_endglobals>> {};

	struct local : seq<opt<key_constant>, key_local, name, opt<key_array>, name, opt<seq<assign, exp>>> {};
	struct local_list : star<sor<local, newline>> {};

	struct action;
	struct action_list : star<sor<action, newline>> {};

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

	struct native_statement : if_must<key_native, must<name, key_takes, args_statement, check_returns, name>> {};


	struct function_name :name {};
	struct function_returns_type : name {};

	struct function_statement : seq<key_function, must<space, function_name, key_takes, space, args_statement, check_returns, space, function_returns_type, newline>>{};

	struct function_block : seq<local_list, action_list> {};

	struct function : if_must<function_statement, function_block, key_endfunction> {};

	struct chunk : sor<type_extends, globals, native_statement, function, newline> {};

	struct jass : star<chunk> {};

	struct grammar : seq<jass, eof> {};


	using data_type = std::string;

	template< typename Type >
	struct container 
	{ 
		typedef std::shared_ptr<Type> object_ptr;

		std::vector<object_ptr> list;
		std::unordered_map <std::string, object_ptr> map;

		bool insert(const std::string& name, object_ptr obj) {
			if (map.find(name) != map.end()) {
				return false;
			}
			list.push_back(obj);
			map.emplace(name, obj);
			return true;
		}

		object_ptr find(const std::string& name) {
			if (map.find(name) == map.end()) {
				return nullptr;
			}
			return map.at(name);
		}

		object_ptr current() {
			return list.back();
		}
	};

	struct state_base {
		std::shared_ptr<std::string> source;
		size_t line;
		size_t column;

		void save_position(std::shared_ptr<std::string> s, const position& p) {
			source = s;
			line = p.line;
			column = p.column;
		}
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
		
		bool is_array;
	};

	struct arg_state : state_base {
		data_type type;
		data_type name;
	};

	struct function_state : state_base
	{
		data_type name;

		container<arg_state> args;
		data_type returns_type;

		container<local_state> locals;
	};

	struct native_state : state_base
	{
		data_type name;

		container<arg_state> args;
		data_type returns_type;
	};

	 
	struct jass_state
	{
		std::shared_ptr<std::string> source;

		container<type_state> types;
		container<global_state> globals;
		container<native_state> natives;
		container<function_state> functions;

		jass_state() {
			//base type

			std::vector<std::string> base_list = { "integer", "real", "string", "boolean", "code", "handle" };

			for (auto& name : base_list) {
				auto obj = std::make_shared<type_state>();
				obj->name = name;
				types.insert(name, obj);
			}
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
		template< typename ParseInput, typename... Args>
		jass_parse_error(ParseInput& in, const std::string& msg, Args... args)
			: parse_error(error_format(msg, args...), in)
		{ }
	};

	template< typename Rule >
	struct check_action {};
	

	//解析type 子类型名 生成l===数据
	template<> struct check_action<type_name>
	{
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s)
		{
			std::string name = in.string();

			auto type = s.types.find(name);
			if (type) {
				throw jass_parse_error(in, "ERROR_REDEFINE_TYPE", name, *type->source, type->line);
			}
	
			type = std::make_shared<type_state>();
			type->name = name;
			type->save_position(s.source, in.position());
			s.types.insert(name, type);
		}
	};

	template<> struct check_action<type_parent_name>
	{
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s)
		{
			std::string name = in.string();

			auto type = s.types.find(name);
			if (!type) {
				throw jass_parse_error(in, "ERROR_UNDEFINE_TYPE", name);
			}

			auto child = s.types.current();
			child->parent = type;
		}
	};



	//解析jass函数名 生成函数数据
	template<> struct check_action<function_name>
	{
		template< typename ActionInput >
		static void apply(const ActionInput & in, jass_state & s)
		{
			std::string name = in.string();
			auto func = s.functions.find(name);
			if (func) {
				throw jass_parse_error(in, "ERROR_REDEFINE_FUNCTION", name, *func->source, func->line);
			} 
			auto f = std::make_shared<function_state>();
			func->name = name;
			func->save_position(s.source, in.position());
			s.functions.insert(name, func);
		}
	};



	
}


#undef ERR
