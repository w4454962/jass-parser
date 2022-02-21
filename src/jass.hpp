﻿#pragma once

#include "stdafx.h"

#define ERR TAO_PEGTL_RAISE_MESSAGE
 
namespace jass {

	using namespace tao::pegtl;

	struct line_char : sor<one<'\r', '\n'>, istring<'\r', '\n'>> {};

	struct comment : if_must<two<'/'>, star<seq<not_at<line_char>, bytes<1>>>> {};

	struct whilespace : sor<one<' ', '\t'>, istring<0xef, 0xbb, 0xbf>> {};

	struct space : star<sor<comment, whilespace>> {};

	struct newline : seq<space, line_char> {
		static constexpr const char* error_message = "MISS_NL";
	};

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

	struct single : sor <one< 'b', 't', 'r', 'n','f', '\\', '"', '\''>, ERR("ERROR_ESC")> {};
	struct escaped : seq<one<'\\'>, single> {};

	struct string : seq<space, one<'"'>, star<sor<escaped, line_char, not_one<'"'>>>, one<'"'>> {};

	struct boolean : sor<key_true, key_false> {};

	struct digits : plus< digit> {};

	struct real : sor<seq<space, opt<one< '-' >>, space, digits, one<'.'>, digits>, seq<one<'.'>, ERR("ERROR_REAL")>> {};


	struct char_1 : sor<escaped, line_char, not_one<'\''>> {};
	struct char_4 : sor<line_char, not_one<'\''>> {};
	struct char_256: sor<
			seq<one<'\''>, char_1, one<'\''>>,
			seq<one<'\''>, char_4, char_4, char_4, char_4, one<'\''>>,
			seq<one<'\''>, ERR("ERROR_INT256_COUNT")>
	> {};
	
	struct integer256 : seq<space, char_256> {};
	struct integer16 : seq<sor<istring<'0', 'x'>, one<'$'>>, sor<plus<xdigit>, ERR("ERROR_INT16")>> {};
	struct integer8 : seq<istring<'0'>, plus<odigit>> {};
	struct integer : seq<space, opt<one< '-' >>, space, sor<integer256, integer16, integer8, digits>> {};


	struct name : seq<alpha, opt<plus<sor<alnum, one<'_'>>>>>{};

	struct code_name : name {};

	struct code : seq<key_function, space, code_name> {};

	struct null : key_null {};
	struct nothing :key_nothing {};
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
	struct pr : seq<space, sor<one<')'>, ERR("ERROR_MISS_PR")>> {};
	struct bl : seq<space, one<'['>> {};
	struct br : seq<space, sor<one<']'>, ERR("ERROR_MISS_BR")>> {};

	struct exp;
	struct exp_unit;

	struct exp_neg_exp;

	struct exp_neg : seq<space, sub, exp_neg_exp> {};
	struct exp_var_name : name {};
	struct exp_var : seq<space, exp_var_name, opt<seq<bl, exp, br>>> {};
	struct exp_value : sor<value, exp_var> {};
	struct exp_call_args : seq<exp, star<seq<comma, exp>>> {};
	struct exp_call_name: name {};
	struct exp_call : seq<space, exp_call_name, pl, opt<exp_call_args>, pr> {};
	struct exp_code : seq<key_function, space, name, pl, opt<exp_call_args>, pr> {};
	struct exp_paren : seq<pl, exp, pr> {};
	struct exp_unit : sor<exp_paren, exp_code, exp_call, exp_value, exp_neg> {};
	struct exp_neg_exp :exp_unit {};

	struct exp_add_sub : sor<add, sub> {};
	struct exp_mul_div : sor<mul, div, mod> {};

	struct exp_not : key_not {};

	struct exp_compare_operator : sor< ue, eq, le, lt, ge, gt, if_must<one<'='>, ERR("ERROR_ASSIGN_AS_EQ")>> {};

	struct exp_or : key_or {};
	struct exp_and : key_and {};

	struct exp_check_mul_div : seq<exp_unit, star<seq<space, exp_mul_div, sor<exp_unit, ERR("ERROR_MISS_EXP")>>>> {};
	struct exp_check_add_sub : seq<exp_check_mul_div, star<seq<space, exp_add_sub, sor<exp_check_mul_div, ERR("ERROR_MISS_EXP")>>>> {};
	struct exp_check_not : seq<space, sor<seq<exp_not, exp_check_add_sub>, seq<exp_not, ERR("ERROR_MISS_EXP")>, exp_check_add_sub>> {};
	struct exp_check_compare : seq<exp_check_not, star<seq<space, exp_compare_operator, sor<exp_check_not, ERR("ERROR_MISS_EXP")>>>> {};
	struct exp_check_or : seq<exp_check_compare, star<seq<space, exp_or, sor<exp_check_compare, ERR("ERROR_MISS_EXP")>>>> {};
	struct exp_check_and : seq<exp_check_or, star<seq<space, exp_and, sor<exp_check_or, ERR("ERROR_MISS_EXP")>>>> {};
	struct exp : exp_check_and {};

	struct type_name: name {};
	struct type_parent_name : name {};
	struct type_extends : if_must<key_type, space, type_name, sor<key_extends, ERR("ERROR_EXTENDS_TYPE")>, space, sor<type_parent_name, ERR("ERROR_EXTENDS_TYPE")>> {};

	struct global_name : name {};
	struct global_type : name {};
	struct global : seq <space, not_at<key_globals, key_function, key_native>, opt<key_constant>, space, global_type, opt<key_array>, space, global_name, opt<seq<assign, exp>>, newline> {};
	struct endglobals: opt<key_endglobals>{};
	struct globals : if_must<key_globals, must<newline, star<sor<global, newline>>, endglobals>> {};

	struct local_name:name {};
	struct local_type:name {};
	struct local : if_must<key_local, space, local_type ,opt<key_array>, space, local_name, opt<seq<assign, exp>>, must<newline>> {};
	struct local_list : star<sor<seq<key_constant, ERR("ERROR_CONSTANT_LOCAL")>, local, newline>> {};

	struct action;
	struct action_list : star<sor<action, newline, seq<key_local, ERR("ERROR_LOCAL_IN_FUNCTION")>>> {};

	struct action_set_var : seq< exp_var, assign, exp>{};

	struct action_call :seq<opt<key_debug>, key_call, sor<exp_call, seq<action_set_var, ERR("ERROR_CALL_AS_SET")>>> {};
	struct action_set :seq<key_set, action_set_var, must<newline>> {};
	struct action_return : seq<key_return, opt<exp>, must<newline>> {};
	struct action_exitwhen : seq<key_exitwhen, exp, must<newline>> {};

	struct then_statement: sor<seq<seq<not_at<key_then>,exp>, key_then>, ERR("ERROR_MISS_THEN")> {};

	struct if_statement : if_must<key_if, then_statement, must<newline>, action_list> {};
	struct elseif_statement : if_must<key_elseif, then_statement, must<newline>, action_list> {};
	struct else_statement : if_must<key_else, must<newline>, action_list> {};

	struct endif: opt<seq<key_endif, must<newline>>>{};

	struct action_if : seq<if_statement, star<elseif_statement>, opt<else_statement>, endif> {};

	struct endloop: opt<seq<key_endloop, must<newline>>>{};
	struct action_loop : if_must<key_loop, must<action_list, endloop>> {};

	struct action : sor<action_call, action_set, action_return, action_exitwhen, action_if, action_loop> {};


	struct arg_type : name {};
	struct arg_name: name {};
	struct arg_statement : seq<space, arg_type, space, arg_name> {};
	struct args_statement : seq < sor < nothing,  seq<arg_statement, star<seq<comma, arg_statement>> >> > {};
	struct check_returns : sor<key_returns, if_must<key_return, ERR("ERROR_RETURN_AS_RETURNS")>> {};

	struct native_name: name{};
	struct returns_type: name {};
	struct native_statement : seq<opt<key_constant>, key_native, must<space, native_name, key_takes, args_statement, check_returns, space, returns_type, newline>> {};

	
	struct function_name :name {};
	struct function_statement : seq<opt<key_constant>, key_function, must<space, function_name, key_takes, space, args_statement, check_returns, space, returns_type, newline>>{};
	
	struct endfunction : opt<key_endfunction> {};

	struct function_block : seq<local_list, action_list, endfunction> {};

	struct function : if_must<function_statement, function_block> {};

	struct ext_action : seq<action, ERR("ERROR_ACTION_IN_CHUNK")>{};

	struct chunk : sor<type_extends, globals, native_statement, function, newline, ext_action> {};

	struct jass : star<chunk> {};

	struct grammar : seq<jass, eof> {};




	struct jass_node :
		parse_tree::basic_node<jass_node>
	{

		std::string_view exp_value_type; //只有在exp节点才有值

		bool has_return = false; //只有在action节点时才有值

		auto sub_string_view(size_t pos) {
			return children[pos]->string_view();
		}

		auto as_sub_input(size_t pos) {
			return children[pos]->as_memory_input();
		}

		template<typename Type>
		auto sub_is_type(size_t pos) {
			return children[pos]->is_type<Type>();
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
		
		std::shared_ptr<local_state> redef_state;

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

		bool is_const;
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

		bool is_action_set = false;
		bool is_action_call = false;

		bool has_function = false;
		bool has_return_any = false;

		std::unordered_map<std::string_view, bool> keyword_map;

		size_t key_globals_line = 0;

		std::stack<size_t> if_line_stack;
		std::stack<size_t> loop_line_stack;

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
				//keyword_map.emplace(name, true);

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
		size_t width;

		template<typename ParseInput, typename... Args>
		jass_parse_error(const ParseInput& in, const std::string_view& msg, Args... args)
			: parse_error(error_format(msg, args...), in),
			width(in.end() - in.begin())
		{ }

		template<typename ParseInput>
		jass_parse_error(const std::string& msg, const ParseInput& in)
			: parse_error(msg, in),
			width(in.end() - in.begin())
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
				if (ptr->parent == nullptr) {
					throw jass_parse_error(input, "ERROR_DEFINE_NATIVE_TYPE", name);
				}

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
			if (auto it = parent->child_map.find("null"); it != parent->child_map.end() && it->second) {
				ts->child_map.emplace("null", true);
			}

			s.types.save(name, ts);
			//s.keyword_map.emplace(name, true);

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

			size_t name_index = 0, args_index = 1, returns_index = 2;
			bool is_const = false;
			if (n->children[0]->is_type<key_constant>()) {
				name_index = 1;
				args_index = 2;
				returns_index = 3;
				is_const = true;
			}

			auto name = n->sub_string_view(name_index);
			if (s.is_keyword(name)) {//名字是关键字
				throw jass_parse_error(n->as_sub_input(name_index), "ERROR_KEY_WORD", name);
			}

			std::vector<void*> check_list = { &s.natives, &s.functions, &s.globals };

			for (auto table : check_list) {
				auto ptr = ((container<state_base>*)table)->find(name);
				if (ptr) { //重复定义
					auto input = n->as_sub_input(name_index);
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
				s.has_function = true;
				s.has_return_any = false;
			}

			ns->name = name;
			ns->save_node(n);

			auto& args = n->children[args_index];
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

			auto return_type = n->sub_string_view(returns_index);
			if (!s.types.find(return_type)) { //返回类型未定义
				throw jass_parse_error(n->as_sub_input(returns_index), "ERROR_UNDEFINE_TYPE", return_type);
			}

			ns->is_const = is_const;
			ns->returns_type = return_type;
		}
	};

	
	//检查global全局变量申明格式
	struct global_content
		: parse_tree::apply< global_content >
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			size_t size = n->children.size();

			auto gs = std::make_shared<global_state>();
			gs->is_array = false;
			gs->is_const = false;
			gs->save_node(n);

			auto check_name = [&](size_t pos) {
				auto& node = n->children[pos];
				auto name = node->string_view();
				if (s.is_keyword(name)) {//全局变量名字是关键字
					throw jass_parse_error(node->as_memory_input(), "ERROR_KEY_WORD", name);
				}

				auto ptr = s.globals.find(name);
				if (ptr) { //全局变量重复定义
					position p = ptr->node->begin();
					throw jass_parse_error(node->as_memory_input(), "ERROR_REDEFINE_GLOBAL", name, p.source, p.line);
				}

				auto fp = s.functions.find(name);
				if (fp) { //全局变量跟函数名重复
					position p = fp->node->begin();
					throw jass_parse_error(node->as_memory_input(), "ERROR_REDEFINE_FUNCTION", name, p.source, p.line);
				}

				auto np = s.natives.find(name);
				if (np) { //全局变量跟native重复
					position p = np->node->begin();
					throw jass_parse_error(node->as_memory_input(), "ERROR_DEFINE_NATIVE_TYPE", name, p.source, p.line);
				}
				return name;
			};

			auto check_type = [&](size_t pos) {
				auto& node = n->children[pos];
				auto type = node->string_view();
				if (!s.types.find(type)) {//全局变量类型不存在
					throw jass_parse_error(node->as_memory_input(), "ERROR_UNDEFINE_TYPE", type);
				}
				return type;
			};

			if (size == 2) { // type name
				gs->type = check_type(0);
				gs->name = check_name(1);
			} else if (size == 3) {
				if (n->sub_is_type<key_constant>(0)) { //当 constant type name 时报错
					throw jass_parse_error(n->as_sub_input(0), "ERROR_CONSTANT_INIT");
				}
				gs->type = check_type(0);

				if (n->sub_is_type<key_array>(1)) { // type array name
					gs->is_array = true;
					gs->name = check_name(2);
				
					if (gs->type == demangle<code>()) { //code类型不能是数组
						throw jass_parse_error(n->as_sub_input(0), "ERROR_CODE_ARRAY");
					}
				} else if (n->sub_is_type<global_name>(1)){ // type name = value
					gs->name = check_name(1);
					auto& exp = n->children[2];
					if (!s.is_extends_type(gs->type, exp->exp_value_type)) { //值类型 跟变量类型不符
						throw jass_parse_error(n->as_sub_input(2), "ERROR_SET_TYPE", gs->name, gs->type, exp->exp_value_type);
					}
				} 
			} else if (size == 4) { //constant type name = value 
				if (n->sub_is_type<key_array>(2)) { //当 constant type array name 时报错
					throw jass_parse_error(n->as_sub_input(0), "ERROR_CONSTANT_INIT");
				}else if (n->sub_is_type<key_array>(1)) { //当 type array name = value 时报错
					throw jass_parse_error(n->as_sub_input(1), "ERROR_ARRAY_INIT");
				}

				gs->type = check_type(1);
				gs->name = check_name(2);
				gs->is_const = true;
				auto& exp = n->children[3];
				if (!s.is_extends_type(gs->type, exp->exp_value_type)) { //值类型 跟变量类型不符
					throw jass_parse_error(n->as_sub_input(3), "ERROR_SET_TYPE", gs->name, gs->type, exp->exp_value_type);
				}
			} else if (size == 5) { // constant type array name = value
				throw jass_parse_error(n->as_sub_input(2), "ERROR_ARRAY_INIT");
			}
			
			s.globals.save(gs->name, gs);
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
				
				if (s.types.find(name)) {//局部变量名字跟类型重复
					throw jass_parse_error(node->as_memory_input(), "ERROR_DEFINE_NATIVE_TYPE", name);
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


			//申请局部变量 能修改同名全局变量的类型
			auto gs = s.globals.find(ls->name);
			if (gs) {
				gs->type = ls->type;
				gs->is_array = ls->is_array;
				gs->redef_state = ls;
			}

			fs->locals.save(ls->name, ls);
		}
	};

	struct exp_content
		: parse_tree::apply< exp_content >
	{
		
		typedef std::unique_ptr<jass_node> node_ptr;


		static std::string_view check_exp_type(node_ptr& n, jass_state& s, node_ptr& first, node_ptr& op, node_ptr& second) {
			std::string str = op->string();
			uint32_t byte = 0;
	
			for (auto it = str.begin(); it != str.end(); it++) {
				byte = (byte << 8) | *it;
			}

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
					throw jass_parse_error(n->as_memory_input(), "ERROR_ADD", t1, t2);
				}
				break;
			case '-':
				exp_type = get_match_exp_type(t1, t2); 
				if (exp_type.empty()) {
					throw jass_parse_error(n->as_memory_input(), "ERROR_SUB", t1, t2);
				}
				break;

			case '*':
				exp_type = get_match_exp_type(t1, t2);
				if (exp_type.empty()) {
					throw jass_parse_error(n->as_memory_input(), "ERROR_MUL", t1, t2);
				}
				break;

			case '/':
				exp_type = get_match_exp_type(t1, t2);
				if (exp_type.empty()) {
					throw jass_parse_error(n->as_memory_input(), "ERROR_DIV", t1, t2);
				}
				break;

			case '%':
			
				if (t1 == demangle<integer>() && t2 == demangle<integer>()) {
					exp_type = t1;
				} else {
					throw jass_parse_error(n->as_memory_input(), "ERROR_MOD");
				}
				break;

			case '==':
			case '!=':
				if (t1 == demangle<null>() || t2 == demangle<null>()) {
					exp_type = demangle<boolean>();
				} else if (!get_match_exp_type(t1, t2).empty()) {
					exp_type = demangle<boolean>();
				} else if (s.get_base_type(t1) == s.get_base_type(t2)) {
					exp_type = demangle<boolean>();
				} else {
					throw jass_parse_error(n->as_memory_input(), "ERROR_EQUAL", t1, t2);
				}
				break;

			case '>':
			case '<':
			case '>=':
			case '<=':
				
				if (!get_match_exp_type(t1, t2).empty()) {
					exp_type = demangle<boolean>();
				} else {
					throw jass_parse_error(n->as_memory_input(), "ERROR_COMPARE", t1, t2);
				}
				break;
			case 'and':
			case 'or':
			
				if (t1 == demangle<boolean>() && t2 == demangle<boolean>()) {
					exp_type = demangle<boolean>();
				} else if (byte == 'and') {
					throw jass_parse_error(n->as_memory_input(), "ERROR_AND", t1, t2);
				} else if (byte == 'or') {
					throw jass_parse_error(n->as_memory_input(), "ERROR_OR", t1, t2);
				}
				break;
			default:
				throw jass_parse_error(n->as_memory_input(), "UNKNOWN_EXP");
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
	 
				n->exp_value_type = check_exp_type(n, s, first, op, second);
			}
		}
	};

	struct exp_not_content
		: parse_tree::apply< exp_not_content >
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			if (n->children.size() == 1) { //没有操作符的匹配
				n = std::move(n->children.front());
				return;
			}

			if (n->children.size() == 0) {
				return;
			}
			auto& op = n->children[0];
			auto& exp = n->children[1];

			if (op->is_type<exp_not>()) {
				if (exp->exp_value_type != demangle<boolean>()) {
					throw jass_parse_error(exp->as_memory_input(), "ERROR_NOT_TYPE");
				}
				n->exp_value_type = exp->exp_value_type;
			}
		}
	};



	//取反类型检查
	struct exp_neg_exp_content
		: parse_tree::apply< exp_neg_exp_content >
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			if (n->children.size() == 1) { //没有操作符的匹配
				n = std::move(n->children.front());
				return;
			}
		}
	};

	//取反类型检查
	struct exp_neg_content
		: parse_tree::apply< exp_neg_content >
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			auto& exp = n->children[1];

			if ((exp->exp_value_type != demangle<real>()) && (exp->exp_value_type != demangle<integer>())) {
				throw jass_parse_error(exp->as_memory_input(), "ERROR_NEG", exp->exp_value_type);
			}
		
			n->exp_value_type = exp->exp_value_type;
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
						throw jass_parse_error(n->as_memory_input(), "ERROR_WASTE_INDEX", name);
					}
					n->exp_value_type = as->type;

					if (s.is_action_set) { //参数set
						s.is_action_set = false;
					}
				} else if (auto ls = fs->locals.find(name); ls) {
					if (has_index && !ls->is_array) { //局部变量不需要索引
						throw jass_parse_error(n->as_memory_input(), "ERROR_WASTE_INDEX", name);

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

					if (s.is_action_set) { //局部变量set
						ls->is_init = true;
						s.is_action_set = false;
					}

					//if (!ls->is_array && !ls->is_init) { //非数组的局部变量 如果没有初始化就引用 进行报错
					//	throw jass_parse_error(n->as_sub_input(0), "ERROR_GET_UNINIT", name);
					//}

					n->exp_value_type = ls->type;

				} else if (auto gs = s.globals.find(name); gs) {
					if (has_index && !gs->is_array) { //全局变量不需要索引
						throw jass_parse_error(n->as_sub_input(0), "ERROR_WASTE_INDEX", name);

					} else if (!has_index && gs->is_array) { //全局数组缺少索引
						throw jass_parse_error(n->as_sub_input(0), "ERROR_NO_INDEX", name);

					} else if (has_index && gs->is_array) {
						auto& exp = n->children[1];

						if (exp->exp_value_type != demangle<integer>()) { //索引类型错误
							throw jass_parse_error(n->as_sub_input(1), "ERROR_INDEX_TYPE", name, exp->exp_value_type);
						}
					}

					if (s.is_action_set) { //全局变量set
						s.is_action_set = false;

						if (gs->is_const) { //修改constant类型的全局变量
							throw jass_parse_error(n->as_sub_input(0), "ERROR_SET_CONSTANT", name);
						}

						if (fs->is_const && !gs->is_const) { //在const 的函数内部调用 非const的全局变量
							throw jass_parse_error(n->as_sub_input(0), "ERROR_SET_IN_CONSTANT", name);
						}

					}
					n->exp_value_type = gs->type;
				} else if (auto fs = s.functions.find(name); fs) {
					throw jass_parse_error(n->as_sub_input(0), "ERROR_SET_AS_CALL");

				} else {
					

					//变量不存在
					throw jass_parse_error(n->as_sub_input(0), "VAR_NO_EXISTS", name);
				}
				
			} else {
				//全局变量申明时初始化赋值
			
				if (auto gs = s.globals.find(name); gs) {
					if (has_index && !gs->is_array) { //全局变量不需要索引
						throw jass_parse_error(n->as_sub_input(0), "ERROR_WASTE_INDEX", name);

					} else if (!has_index && gs->is_array) { //全局数组缺少索引
						throw jass_parse_error(n->as_sub_input(0), "ERROR_NO_INDEX", name);

					} else if (has_index && gs->is_array) {
						auto& exp = n->children[1];

						if (exp->exp_value_type != demangle<integer>()) { //索引类型错误
							throw jass_parse_error(n->as_sub_input(1), "ERROR_INDEX_TYPE", name, exp->exp_value_type);
						}
					}
					n->exp_value_type = gs->type;
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

			if (s.is_function_block) {//在函数内
				auto in_func = s.functions.back();

				if (in_func->is_const && !f->is_const) { //在常量函数里 不能调用非常量函数 
					throw jass_parse_error(n->as_sub_input(0), "ERROR_CALL_IN_CONSTANT", name);
				}

			} else { //在函数外 也就是初始化全局变量的时候
				switch (hash_s(name))
				{
				case "OrderId"s_hash:
				case "OrderId2String"s_hash:
				case "UnitId2String"s_hash:
					//这几个会返回null 
					throw jass_parse_error(n->as_sub_input(0), "WARNING_NULL_NATIVE_IN_GLOBAL", name);
					break;
				case "GetObjectName"s_hash:
				case "CreateQuest"s_hash:
				case "CreateMultiboard"s_hash:
				case "CreateLeaderboard"s_hash:
					//这几个会崩溃
					throw jass_parse_error(n->as_sub_input(0), "WARNING_CRASH_NATIVE_IN_GLOBAL", name);
					break;
				default:
					break;
				}
			}

			n->exp_value_type = f->returns_type;

		}
	};

	//检查set 变量
	struct action_set_content
		: parse_tree::apply<action_set_content>
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			auto& var = n->children[0];
			auto& exp = n->children[1];

			if (!s.is_extends_type(var->exp_value_type, exp->exp_value_type)) { //值类型 跟变量类型不符
				throw jass_parse_error(exp->as_memory_input(), "ERROR_SET_TYPE", var->sub_string_view(0), var->exp_value_type, exp->exp_value_type);
			}

		}
	};




	//生成返回值类型 
	struct action_return_content
		: parse_tree::apply<action_return_content>
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			std::string_view return_type = demangle<nothing>();


			if (n->children.size() > 0) {
				auto& exp = n->children[0];
				return_type = exp->exp_value_type;
			}

			//检查当前函数的返回类型是否一致
			auto fs = s.functions.back();

			if (fs->returns_type != return_type) {
				if (return_type == demangle<nothing>()) { //函数需要返回类型 但是返回空 
					throw jass_parse_error(n->as_memory_input(), "ERROR_MISS_RETURN", fs->name, fs->returns_type);

				} else if (fs->returns_type == demangle<nothing>()) { //函数不需要返回类型 但是 返回非空
					throw jass_parse_error(n->as_memory_input(), "ERROR_WASTE_RETURN", fs->name, return_type);

				} else if (fs->returns_type == demangle<real>() && return_type == demangle<integer>()) {
					//函数需要返回实数 但是却返回整数  结果是0.0
					throw jass_parse_error(n->as_memory_input(), "ERROR_RETURN_INTEGER_AS_REAL", fs->name, fs->returns_type, return_type);

				} else if (!s.is_extends_type(fs->returns_type, return_type)) {

					throw jass_parse_error(n->as_memory_input(), "ERROR_RETURN_TYPE", fs->name, fs->returns_type, return_type);
				}
			}

			//标记有返回值
			n->has_return = true;
			s.has_return_any = true;
		}
	};

	//检查动作块里是否有返回值类型
	struct action_list_content
		: parse_tree::apply<action_list_content>
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			for (auto& action : n->children) {
				if (action->has_return) {
					n->has_return = action->has_return;
					break;
				}
			}
		}
	};


	//检查exitwhen 
	struct action_exitwhen_content
		: parse_tree::apply<action_exitwhen_content>
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			auto& exp = n->children[1];

			if (exp->exp_value_type != demangle<boolean>()) { // exp类型不是boolean
				throw jass_parse_error(exp->as_memory_input(), "ERROR_CONDITION_TYPE");
			}

			if (s.loop_line_stack.empty()) {
				throw jass_parse_error(n->as_sub_input(0), "ERROR_EXITWHEN");
			}
		}
	};

	
	struct if_statement_content
		: parse_tree::apply<if_statement_content>
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			size_t actions_index;

			if (n->children.size() == 2) {//检查if elseif 的条件类型

				auto& exp = n->children[0];

				if (exp->exp_value_type != demangle<boolean>()) { //exp类型不是boolean
					throw jass_parse_error(exp->as_memory_input(), "ERROR_CONDITION_TYPE");
				}
				actions_index = 1;
			} else {
				actions_index = 0;
			}
		
			auto& actions = n->children[actions_index]; 

			n->has_return = actions->has_return; // 将动作列表里的返回标记 向上传递给if分支
		}
	};

	struct action_if_content
		: parse_tree::apply<action_if_content>
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{
			size_t count = 0;

			jass_node* last = nullptr;

			size_t size = n->children.size();
			//统计if的所有分支 返回标记数量
			for (size_t i = 0; i < size; i++) {
				auto& node = n->children[i];
				if (node->has_return) {
					count++;
					last = node.get();
				}
			}

			//所有分支都有返回标记 && 包含else分支
			if (count == size && last && last->is_type<else_statement>()) {
				n->has_return = true;
			}
		}

	};
 



	//当函数里所有动作都分析完毕 检查是否有返回类型
	struct function_block_content
		: parse_tree::apply<function_block_content>
	{
		template<typename ParseInput>
		static void transform(const ParseInput& in, std::unique_ptr<jass_node>& n, jass_state& s)
		{

			size_t size = n->children.size();
			size_t actions_index = size - 2, endfunction_index = size - 1;

		
			std::string_view return_type = demangle<nothing>();

			auto fs = s.functions.back();


			bool has_return = false;

			if (size > 1) {
				auto& actions = n->children[actions_index];

				//遍历函数的所有动作 只要其中之一有返回标记 当前函数就有返回值
				for (auto& node : actions->children) {
					if (node->has_return) {
						has_return = true;
						break;
					}
				}
			}
		
			if (!has_return && fs->returns_type != demangle<nothing>()) { //没有返回值 且函数返回类型不是nothing

				if (s.has_return_any) {
					throw jass_parse_error(n->as_sub_input(endfunction_index), "ERROR_RETURN_IN_ALL", fs->name, fs->returns_type);
				} else {
					throw jass_parse_error(n->as_sub_input(endfunction_index), "ERROR_MISS_RETURN", fs->name, fs->returns_type);
				}
			}
		}
	};

	template<typename Rule>
	using selector = parse_tree::selector<
		Rule,

	
		parse_tree::store_content::on<
			type_name,
			type_parent_name,

			function_name,
			native_name,
			returns_type,

			args_statement,
				arg_statement,
					arg_name,
					arg_type,

			key_constant,
			global_type,
			global_name,
			key_array,

			local_type,
			local_name,

			add, sub, mul, div, mod, gt, ge, lt, le, eq, ue, exp_or, exp_and, exp_not,

			exp_var_name,
			exp_call_name,
			exp_call_args,

			key_exitwhen,
			key_endfunction
		>,

		type_extends_content::on<type_extends>,

		function_statement_content::on<native_statement, function_statement>,

		global_content::on<global>,
	
		local_content::on<local>,
	
		exp_content::on<
			exp,
			exp_check_and,
			exp_check_or,
			exp_check_compare,
			exp_check_add_sub,
			exp_check_mul_div
		>,


		exp_not_content::on<exp_check_not>,
	
		exp_neg_content::on<exp_neg>,

		exp_neg_exp_content::on<exp_neg_exp>,
		
		value_content::on<null, boolean, string, real, integer, code>,

		var_content::on<exp_var>,

		exp_call_content::on<exp_call>,

		action_set_content::on<action_set>,

		action_exitwhen_content::on<action_exitwhen>,

		action_return_content::on<action_return>,

		action_list_content::on<action_list>,

		if_statement_content::on<if_statement, elseif_statement, else_statement>,

		action_if_content::on<action_if>, 

		function_block_content::on<function_block>

	>;

	
	template< typename Rule >
	struct check_action 
		:tao::pegtl::nothing<Rule>
	{};



	template<>
	struct check_action<local_list> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {
			s.is_local_block = false;
		}
	};

	template<>
	struct check_action<key_set> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {
			s.is_action_set = true;
		}
	};

	template<>
	struct check_action<key_call> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {
			s.is_action_call = true;
		}
	};
	
	

	template<>
	struct check_action<code_name> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {
			auto name = in.string_view();
	
			auto fs = s.functions.find(name);
			if (!fs) {
				throw jass_parse_error(in, "FUNCTION_NO_EXISTS", name);
			}
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
	struct check_action<endfunction> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {
			
			if (in.string_view().size() == 0) {
				auto fs = s.functions.back();
				throw jass_parse_error(in, "ERROR_ENDFUNCTION", std::to_string(fs->node->begin().line));
			}
			s.is_function_block = false;
		}
	};
	

	template<>
	struct check_action<key_globals> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {
			if (s.has_function) {
				throw jass_parse_error(in, "ERROR_GLOBAL_AFTER_FUNCTION");
			}
			s.key_globals_line = in.position().line;
		}
	};

	template<>
	struct check_action<endglobals> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {

			if (in.string_view().size() == 0) {
				throw jass_parse_error(in, "ERROR_ENDGLOBALS", std::to_string(s.key_globals_line));
			}

		}
	};

	template<>
	struct check_action<key_if> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {
			s.if_line_stack.push(in.position().line);
		}
	};

	template<>
	struct check_action<endif> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {

			if (in.string_view().size() == 0) {
				throw jass_parse_error(in, "ERROR_ENDIF", std::to_string(s.if_line_stack.top()));
			}
			s.if_line_stack.pop();
		}
	};

	template<>
	struct check_action<key_loop> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {
			s.loop_line_stack.push(in.position().line);
		}
	};

	template<>
	struct check_action<endloop> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {

			if (in.string_view().size() == 0) {
				throw jass_parse_error(in, "ERROR_ENDLOOP", std::to_string(s.loop_line_stack.top()));
			}
			s.loop_line_stack.pop();
		}
	};


	template<>
	struct check_action<char_256> {
		template< typename ActionInput >
		static void apply(const ActionInput& in, jass_state& s) {
			std::string_view str = in.string_view();

			if (str.size() < 2) {
				return;
			}

			std::string_view code = std::string_view(str.begin() + 1, str.end() - 1);

			for (auto it = code.begin(); it != code.end(); it++) {
				if (*it == '\\') {
					throw jass_parse_error(in, "ERROR_INT256_ESC");
				}
			}
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

		(void)normal< line >::match< apply_mode::nothing, rewind_mode::dontcare, tao::pegtl::nothing, normal >(in2);

		return std::string_view(b, static_cast<std::size_t>(in2.current() - b));
	}

}


#undef ERR

#undef MACRO_DEF
#undef KEYWORD_ALL
#undef KEYWORD
#undef MACRO_INPUT