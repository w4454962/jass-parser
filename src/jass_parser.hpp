#pragma once
#include "stdafx.h"
#include "jass_peg_rule.h"

std::string_view convert_message(std::string_view msg);


template<typename... Args>
inline std::string error_format(std::string_view msg, Args... args) {
	return std::vformat(convert_message(msg), _STD make_format_args(args...));
}


typedef std::shared_ptr<struct Node> NodePtr;
typedef std::shared_ptr<struct VarBaseNode> VarPtr;
typedef std::shared_ptr<struct ExpNode> ExpPtr;
typedef std::shared_ptr<struct ActionNode> ActionPtr;


template<typename N, typename T>
inline auto CastNode(const T& v) {
	return std::dynamic_pointer_cast<N>((NodePtr)v);
}
using string_t = std::string;


template< typename Type >
struct Container
{
	typedef std::shared_ptr<Type> object_ptr;

	std::vector<object_ptr> list;
	std::unordered_map <string_t, object_ptr> map;

	bool save(const string_t& name, object_ptr obj) {
		if (map.find(name) != map.end()) {
			return false;
		}
		list.push_back(obj);
		map.emplace(name, obj);
		return true;
	}

	object_ptr find(const string_t& name) {
		if (map.find(name) == map.end()) {
			return nullptr;
		}
		return map.at(name);
	}

	object_ptr back() {
		return list.back();
	}
};

enum class ValueType {
	nothing		= 0,
	unknow		= 1,
	null		= 2,
	code		= 3,
	integer		= 4,
	real		= 5,
	string		= 6,
	handle		= 7,
	boolean		= 8
};


typedef std::function<bool(NodePtr node, int branch_index)> NodeFilter;

struct Node {
	string_t type = "none";

	virtual const string_t get_type (){  return type; }
	virtual void each_childs(const NodeFilter& filter) { }
	virtual size_t get_childs_size() { return 0; }
};



struct VarBaseNode : Node {
	string_t vtype;
	string_t name;

	bool has_set;

	VarBaseNode() {
		type = "var";
		has_set = false;
	}
};


struct ExpNode: Node {
	string_t vtype = "nothing";

	ExpNode() {
		type = "exp";
	}
};

struct ExpValue : ExpNode {

	ExpValue() {
		type = "exp_value";
	}

};

struct ExpNullValue : ExpValue {

	ExpNullValue() {
		vtype = "null";
	}
};

struct	ExpBooleanValue:ExpNode {
	bool value;

	ExpBooleanValue(bool value_)
		: value(value_)
	{ 
		vtype = "boolean";
	}
};

struct ExpStringValue :ExpNode {
	string_t value;
	ExpStringValue(const string_t& value_)
		: value(value_)
	{ 
		vtype = "string";
	}
};

struct ExpRealValue :ExpNode {
	float value;
	ExpRealValue(float value_)
		: value(value_)
	{ 
		vtype = "real";
	}
};

struct ExpIntegerValue :ExpNode {
	uint32_t value;
	ExpIntegerValue(uint32_t value_) 
		: value(value_)
	{ 
		vtype = "integer";
	}

};



struct ExpCodeValue : ExpNode {
	string_t value;
	ExpCodeValue(const string_t& value_)
		: value(value_)
	{
		vtype = "code";
	}
};


struct ExpVar :ExpNode {
	VarPtr var;

	ExpVar(VarPtr& var_)
		:var(var_)
	{
		type = "exp_var";
		vtype = var->type;
	}
};

struct ExpVari : ExpNode {
	VarPtr var;

	ExpPtr index;

	ExpVari(VarPtr& var_, ExpPtr& index_)
		:var(var_),
		index(index_)
	{
		type = "exp_vari";
		vtype = var->type;
	}
};

struct ExpNeg : ExpNode {
	ExpPtr exp;

	ExpNeg(ExpPtr& exp_)
		: exp(exp_)
	{
		type = "exp_neg";
		vtype = exp_->vtype;;
	}
};


struct ExpBinary : ExpNode {

	string_t op;

	ExpPtr first;
	ExpPtr second;

	ExpBinary(const string_t& vtype_, ExpPtr& first_, const string_t& op_, ExpPtr& second_)
		: op(op_),
		first(first_),
		second(second_)
	{
		type = "exp_binary";
		vtype = vtype_;
	}
};

struct ExpUnary :ExpNode {
	string_t op;
	ExpPtr first;

	ExpUnary(const string_t& vtype_, ExpPtr& first_, const string_t& op_)
		: op(op_),
		first(first_)
	{
		type = "exp_unary";
		vtype = vtype_;
	}
};


struct ExpCall : ExpNode {
	string_t name;
	std::vector<ExpPtr> params;

	bool is_action;

	ExpCall(const string_t& func_name_, const string_t& func_return_type)
		: name(func_name_)
	{
		type = "exp_call";
		vtype = func_return_type;
		is_action = false;
	}
};




struct ActionNode :Node {
	bool has_return;
	
	ActionNode() {
		type = "action";
		has_return = false;
	}

};

struct ActionSet : ActionNode {
	string_t name;

	ExpPtr exp;

	ActionSet(const string_t& name_, ExpPtr& exp_)
		: name(name_),
		exp(exp_)
	{
		type = "action_set";
	}
};


struct ActionSetIndex : ActionNode {
	string_t name;
	ExpPtr index;
	ExpPtr exp;

	ActionSetIndex(const string_t& name_, ExpPtr& index_, ExpPtr& exp_)
		: name(name_),
		index(index_),
		exp(exp_)
	{
		type = "action_setarray";
	}
};

struct ActionReturn : ActionNode {
	ExpPtr value;

	ActionReturn(ExpPtr exp_) 
		:value(exp_)
	{
		type = "action_return";
		has_return = true;
	}
};

struct ActionExit : ActionNode {
	ExpPtr exp;

	ActionExit(ExpPtr exp_)
		:exp(exp_)
	{
		type = "action_exitwhen";
	}
};

struct IfNode : ActionNode {
	enum class TYPE : size_t
	{
		IF,
		ELSEIF,
		ELSE
	};

	string_t file;
	size_t line;
	TYPE iftype;
	ExpPtr condition;
	std::vector<ActionPtr> actions;

	IfNode(TYPE iftype_, const string_t& file_, size_t line_)
		:iftype(iftype_),
		file(file_),
		line(line_)
	{
		type = "if";
	}
};

struct ActionIf : ActionNode {
	std::vector<std::shared_ptr<IfNode>> if_nodes;

	ActionIf() {
		type = "action_if";
	}

	virtual void each_childs(const NodeFilter& filter) override {
		for (auto& node : if_nodes) {
			size_t brnch_index = (size_t)node->iftype;
			for (auto& action : node->actions) {
				if (filter(action, brnch_index))  return;
			}
		}
	}

	virtual size_t get_childs_size() override {
		size_t size = 0;

		for (auto& node : if_nodes) {
			size += node->actions.size();
		}
		return size; 
	}
};

struct ActionLoop :ActionNode {
	std::vector<ActionPtr> actions;

	size_t endline;

	ActionLoop(size_t endline_)
		:endline(endline_)
	{
		type = "action_loop";
	}

	virtual void each_childs(const NodeFilter& filter) override {
		size_t brnch_index = 0;
		for (auto& action : actions) {
			if (filter(action, brnch_index))  return;
		}
	}

	virtual size_t get_childs_size() override {
		return actions.size();
	}
};



struct TypeNode : Node {
	string_t file;
	size_t line;
	string_t name;
	std::shared_ptr<TypeNode> parent;
	std::set<string_t> childs;

	TypeNode(const string_t& name_, std::shared_ptr<TypeNode> parent_, const string_t& file_, size_t line_)
		: name(name_),
		parent(parent_),
		file(file_),
		line(line_)
	{ 
		type = "type";
	}
};



struct ArgNode : VarBaseNode {

	ArgNode(const string_t& type_, const string_t& name_)
	{ 
		vtype = type_;
		name = name_;
		type = "var_arg";
		has_set = true;
	}
};

struct LocalNode : VarBaseNode {
	string_t file;
	size_t line;
	
	bool is_array;

	ExpPtr exp;

	LocalNode(const string_t& type_, const string_t& name_, bool is_array_, const string_t& file_, size_t line_)
		: is_array(is_array),
		file(file_),
		line(line_)
	{
		vtype = type_;
		name = name_;
		type = "var_local";
	}
};

struct GlobalNode : LocalNode {

	bool is_const;

	GlobalNode(bool constant, const string_t& type_, const string_t& name_, bool is_array_, const string_t& file_, size_t line_)
		: LocalNode(type_, name, is_array, file_, line_)
	{
		type = "var_global";
		is_const = constant;
	}
};

struct GlobalsNode : Node {

	Container<GlobalNode>* globals;

	GlobalsNode() {
		type = "globals";
		globals = nullptr;
	}

	virtual void each_childs(const NodeFilter& filter) override {
		if (!globals) return;

		size_t brnch_index = 0;
		for (auto& global : globals->list) {
			if (filter(global, brnch_index))  return;
		}
	}

	virtual size_t get_childs_size() override {
		if (!globals) return 0;
		return globals->list.size();
	}
};


struct NativeNode : Node {
	string_t file;
	size_t line;
	bool is_const;
	string_t name;
	string_t returns;
	Container<ArgNode> args;

	NativeNode(bool is_const_, const string_t& name_, const string_t& returns_, const string_t& file_, size_t line_)
		: is_const(is_const_),
		name(name_),
		returns(returns_),
		file(file),
		line(line_)
	{
		type = "native";
	}
};

struct FunctionNode : NativeNode {
	Container<LocalNode> locals;

	std::vector<ActionPtr> actions;

	FunctionNode(bool is_const_, const string_t& name_, const string_t& returns_, const string_t& file_, size_t line_)
		: NativeNode(is_const_, name_, returns_, file_, line_)
	{
		type = "function";
	}

	virtual void each_childs(const NodeFilter& filter) override {
		size_t brnch_index = 0;
		for (auto& local : locals.list) {
			if (filter(local, brnch_index))  return;
		}

		for (auto& action : actions) {
			if (filter(action, brnch_index))  return;
		}
	}

	virtual size_t get_childs_size() override {
		return locals.list.size() + actions.size();
	}
};

struct Jass : Node {
	string_t file;

	Container<TypeNode> types;
	Container<GlobalNode> globals;
	Container<NativeNode> natives;
	Container<FunctionNode> functions;

	Jass(const string_t& file_)
		:file(file_)
	{
		type = "jass";

		//基础数据类型
		auto base_list = { "integer", "real", "string", "boolean", "code", "handle", "nothing" };
		for (auto name : base_list) {
			auto obj = std::make_shared<TypeNode>(name, nullptr, "<init>", 0);
			types.save(name, obj);
			if (name == "code" || name == "string" || name == "handle") { //这些类型可以使用null
				obj->childs.emplace("null");
			}
			else if (name == "real") { // real类型 可以使用integer
				obj->childs.emplace("integer");
			}
		}
	}
};


enum class ErrorLevel : uint8_t {
	error,
	warning
};

struct ParseErrorMessage
{
	ErrorLevel level;
	string_t file;
	string_t message;
	size_t line;
	size_t column;
};


typedef std::shared_ptr<ParseErrorMessage> MsgPtr;
struct ParseLog {
	std::vector<MsgPtr> errors;
	std::vector<MsgPtr> warnings;

	template<typename... Args>
	void error(MsgPtr msg, const std::string_view& fmt, Args... args) {
		msg->message = std::vformat(fmt, _STD make_format_args(args...));
		msg->level = ErrorLevel::error;
		errors.push_back(msg);
	}

	template<typename... Args>
	void warning(MsgPtr msg, const std::string_view& fmt, Args... args) {
		msg->message = std::vformat(fmt, _STD make_format_args(args...));
		msg->level = ErrorLevel::warning;
		warnings.push_back(msg);
	}
};



struct ParseResult {
	

	std::shared_ptr<Jass> jass;
	
	std::unordered_map<size_t, string_t> comments;

	ParseLog log;

};


std::set<string_t> keywords = {
	"globals", "endglobals", "constant", "native", "array", "and",
	"or", "not", "type", "extends", "function", "endfunction", "nothing",
	"takes", "returns", "call", "set", "return", "if", "then", "endif", "elseif",
	"else", "loop", "endloop", "exitwhen", "local", "true", "false"
};

NodePtr null_value(std::make_shared<ExpNullValue>());
NodePtr true_value(std::make_shared<ExpBooleanValue>(true));
NodePtr false_value(std::make_shared<ExpBooleanValue>(false));

void jass_parser(sol::state& lua, const string_t& script, ParseResult& result) {

	lua_State* L = lua.lua_state();

	sol::function make_peg = lua.require_file("peg", "peg");

	sol::function peg_parser = make_peg(jass_peg_rule);

	sol::table parser = lua.create_table();
	
	size_t linecount = 0, column = 0, loop_stack_count = 0;

	string_t file = "war3map.j";

	result.jass = std::make_shared<Jass>(file);

	auto& comments = result.comments;
	auto& functions = result.jass->functions;
	auto& types = result.jass->types;
	auto& globals = result.jass->globals;
	auto& natives = result.jass->natives;
	auto& log = result.log;

	auto position = [&]() {
		auto msg = std::make_shared<ParseErrorMessage>();
		msg->file = file;
		msg->line = linecount;
		msg->column = 0;
		return msg;
	};

	auto check_name = [&](const string_t& name) {
		if (keywords.find(name) != keywords.end()) {
			log.error(position(), "ERROR_KEY_WORD", name);
		}
	};

	auto check_new_name_types = [&](const string_t& name) {
		auto type = types.find(name);
		if (type) {
			if (!type->parent) {
				log.error(position(), "ERROR_REDEFINE_TYPE", name, type->file, type->line);
			}
			else {
				log.error(position(), "ERROR_DEFINE_NATIVE_TYPE", name);
			}
		}
	};
	auto check_new_name_globals = [&](const string_t& name) {
		auto global = globals.find(name);
		if (global) {
			log.error(position(), "ERROR_REDEFINE_GLOBAL", name, global->file, global->line);
		}
	};

	auto check_new_name_functions = [&](const string_t& name) {
		auto func = functions.find(name);
		if (func) {
			log.error(position(), "ERROR_REDEFINE_FUNCTION", name, func->file, func->line);
		}

		auto native = natives.find(name);
		if (native) {
			log.error(position(), "ERROR_REDEFINE_FUNCTION", name, native->file, native->line);
		}
		
	};

	auto check_new_name = [&](const string_t& name) {
		check_new_name_types(name);
		check_new_name_globals(name);
		check_new_name_functions(name);
	};

	auto check_type = [&](const string_t& type) {
		if (!type.empty() && !types.find(type)) {
			log.error(position(), "ERROR_UNDEFINE_TYPE", type);
		}
	};

	auto is_extends = [&](const string_t& type_name, const string_t& parent_name) {
		auto type = types.find(type_name);
		auto parent = types.find(parent_name);

		if (!type || !parent) {
			return true;
		}

		if (type->name == parent->name) {
			return true;
		}
		return parent->childs.find(type_name) != parent->childs.end();
	};

	auto get_function = [&](const string_t& name) {
		check_name(name);

		std::shared_ptr<NativeNode> func;
		
		func = natives.find(name);
		if (!func) {
			func = functions.find(name);
			if (!func) {
				log.error(position(), "FUNCTION_NO_EXISTS", name);
			}
		}
		return func;
	};


	auto check_call = [&](const std::shared_ptr<NativeNode>& func, const std::shared_ptr<ExpCall>& call) {
		if (func->name.empty()) {
			return;
		}
		size_t args_count = func->args.list.size();
		size_t params_count = call->params.size();

		if (args_count > 0) {
			if (args_count != params_count) {
				if (args_count > params_count) {
					//参数数量不一致
					std::string miss;
					for (size_t i = params_count; i < args_count; i++) {
						auto& arg = func->args.list[i];
						if (!miss.empty())
							miss += ",";
						miss += std::format("{} {}", arg->type, arg->name);
					}
					log.error(position(), "ERROR_LESS_ARGS", func->name, args_count, params_count, miss);
				}
				else {
					log.error(position(), "ERROR_MORE_ARGS", func->name, args_count, params_count);
				}
			} else {
				for (size_t i = 0; i < args_count; i++) {
					auto& arg = func->args.list[i];
					auto& exp = call->params[i];
					if (arg->vtype != exp->vtype) {
						if (!is_extends(exp->vtype, arg->vtype)) {
							log.error(position(), "ERROR_WRONG_ARG", func->name, i, arg->vtype, exp->vtype);
						}
					}
				}
			}
			
		} else if (params_count > 0) {
			log.error(position(), "ERROR_WASTE_ARGS", func->name, params_count);
		} 
	};

	auto get_variable = [&](const string_t& name) {
		check_name(name);

		auto func = functions.back();
		if (func) {
			auto local = func->locals.find(name);

			if (local) {
				return (VarPtr)local;
			}

			auto arg = func->args.find(name);
			if (arg) {
				return (VarPtr)arg;
			}
		}
		return (VarPtr)globals.find(name);
	};


	auto check_set = [&](VarPtr var, bool need_array, ExpPtr index, ExpPtr exp) {
		if (!var) return;
		auto& name = var->name;

		if (need_array) {
			if (var->type == "var_arg" || !CastNode<LocalNode>(var)->is_array) {
				log.error(position(), "ERROR_WASTE_INDEX", name);
			}
		} else {
			if (var->type != "var_arg" && CastNode<LocalNode>(var)->is_array) {
				log.error(position(), "ERROR_NO_INDEX", name);
			}
		}

		if (index && !is_extends(index->vtype, "integer")) {
			log.error(position(), "ERROR_INDEX_TYPE", name, index->vtype);
		}

		auto func = functions.back();
		if (var->type == "var_global" && CastNode<GlobalNode>(var)->is_const &&func) {
			log.error(position(), "ERROR_SET_CONSTANT", name);
		}

		if (exp && is_extends(exp->vtype, var->vtype)) {
			log.error(position(), "ERROR_SET_TYPE", name, var->type, exp->vtype);
		}
	};

	auto check_get = [&](VarPtr var, bool need_array) {
		if (!var) return;

		auto& name = var->name;

		if (need_array) {
			if (var->type == "var_arg" || !CastNode<LocalNode>(var)->is_array ) {
				log.error(position(), "ERROR_WASTE_INDEX", name);
			}
		} else {
			if (var->type != "var_arg" && CastNode<LocalNode>(var)->is_array) {
				log.error(position(), "ERROR_NO_INDEX", name);
			}
			if (!var->has_set) {
				log.warning(position(), "ERROR_GET_UNINIT", name);
			}
		}
	};

	auto check_local_with_args = [&](const string_t& name, const string_t& type, bool is_array) {
		auto func = functions.back();
		if (!func) return;
		auto var = func->args.find(name);
		if (!var) return; 

		if (is_array) {
			log.error(position(), "ERROR_REDEFINE_ARRAY_WITH_ARG", func->name, func->file, func->line);
			return;
		}

		if (type != var->vtype) {
			log.error(position(), "ERROR_REDEFINE_ARRAY_WITH_ARG", name, type, func->name, var->vtype, func->file, func->line);
			return;
		}
		log.warning(position(), "ERROR_REDEFINE_ARG", name, func->file, func->line);
	};


	auto check_local_with_globals = [&](const string_t& name, const string_t& type, bool is_array) {
		auto var = globals.find(name);
		if (!var) return;

		if (is_array && !var->is_array) {
			log.error(position(), "ERROR_REDEFINE_ARRAY_WITH_GLOBAL", name, var->file, var->line);
		} else {
			log.warning(position(), "ERROR_REDEFINE_GLOBAL", name, var->file, var->line);
		}
	};

	auto get_match_exp_type = [](const string_t& t1, const string_t& t2) {
		string_t exp_type;
		if (t1 == "integer") {
			if (t2 == "integer") {
				exp_type = "integer";
			} else if (t2 == "real") {
				exp_type = "real";
			}
		} else if (t1 == "real") {
			if (t2 == "integer" || t2 == "real") {
				exp_type = "real";
			}
		}
		return exp_type;
	};

	auto get_connect_exp_type = [](const string_t& t1, const string_t& t2) {
		if ((t1 == "string" || t1 == "null") && (t2 == "string" || t2 == "null")) {
			return "string";
		}
		return "";
	};

	auto is_base_type_eq = [&](const string_t& t1, const string_t& t2) {
		auto first = types.find(t1);
		while (first && first->parent) {
			first = first->parent;
		}
		auto second = types.find(t2);
		while (second && second->parent) {
			second = second->parent;
		}
		if (!first || !second) {
			return false;
		}
		return first->name == second->name;
	};

	auto get_two_exp_type = [&](ExpPtr& first, const string_t& op, ExpPtr& second) {
		uint32_t byte = 0;
		for (auto c : op) { 
			byte = (byte << 8) | c; 
		}

		string_t t1, t2, exp_type = "nothing";

		t1 = first->vtype;
		t2 = second->vtype;

		switch (byte)
		{
		case '+':
			exp_type = get_match_exp_type(t1, t2); //如果是数字相加
		
			if (exp_type.empty()) { //否则 如果是 字符串连接
				exp_type = get_connect_exp_type(t1, t2);
			}
			if (exp_type.empty()) {
				log.error(position(), "ERROR_ADD", t1, t2);
			}
			break;
		case '-':
			exp_type = get_match_exp_type(t1, t2);
			if (exp_type.empty()) {
				log.error(position(), "ERROR_SUB", t1, t2);
			}
			break;
		
		case '*':
			exp_type = get_match_exp_type(t1, t2);
			if (exp_type.empty()) {
				log.error(position(), "ERROR_MUL", t1, t2);
			}
			break;
		
		case '/':
			exp_type = get_match_exp_type(t1, t2);
			if (exp_type.empty()) {
				log.error(position(), "ERROR_DIV", t1, t2);
			}
			break;
		
		case '%':
			if (t1  == "integer" && t2 == "integer") {
				exp_type = t1;
			} else {
				log.error(position(), "ERROR_MOD");
			}
			break;
		
		case '==':
		case '!=':
			if (t1 == "null" || t2 == "null") {
				exp_type = "boolean";
			} else if (!get_match_exp_type(t1, t2).empty()) {
				exp_type = "boolean";
			} else if (is_base_type_eq(t1, t2)) {
				exp_type = "boolean";
			} else {
				log.error(position(), "ERROR_EQUAL", t1, t2);
			}
			break;
		case '>':
		case '<':
		case '>=':
		case '<=':
		
			if (!get_match_exp_type(t1, t2).empty()) {
				exp_type = "boolean";
			} else {
				log.error(position(), "ERROR_COMPARE", t1, t2);
			}
			break;
		case 'and':
		case 'or':
			if (t1 == "boolean" && t2 == "boolean") {
				exp_type = "boolean";
			} else if (byte == 'and') {
				log.error(position(), "ERROR_AND", t1, t2);
			} else if (byte == 'or') {
				log.error(position(), "ERROR_OR", t1, t2);
			}
			break;
		default:
			log.error(position(), "UNKNOWN_EXP");
			break;
		}
		return exp_type;
	};


	parser["nl"] = [&]() {
		linecount++;
	};

	parser["File"] = []() {
		return "file";
	};

	parser["Line"] = [&]() {
		return linecount;
	};

	parser["Comment"] = [&](const string_t& str) {
		comments[linecount] = str;
	};

	parser["NULL"] = [] {
		return null_value;
	};

	parser["TRUE"] = [] {
		return true_value;
	};

	parser["FALSE"] = [] {
		return false_value;
	};

	parser["String"] = [](const string_t& str) {
		return (NodePtr)std::make_shared<ExpStringValue>(str);
	};
	
	parser["Real"] = [](const std::string& str) {
		return (NodePtr)std::make_shared<ExpRealValue>(std::stof(str));
	};

	parser["Integer8"] = [](const std::string& neg, const std::string& str) {
		return (NodePtr)std::make_shared<ExpIntegerValue>(std::stoul(neg + str, 0, 8));
	};

	parser["Integer10"] = [](const std::string& neg, const std::string& str) {
		return (NodePtr)std::make_shared<ExpIntegerValue>(std::stoul(neg + str, 0, 10));
	};

	parser["Integer16"] = [](const std::string& neg, const std::string& str) {
		return std::make_shared<ExpIntegerValue>(std::stoul(neg + str, 0, 16));
	};

	parser["Integer256"] = [](const string_t& neg, const string_t& str) {
		int i = 0;
		if (str.length() == 1) {
			i = str[0];
		} else if (str.length() == 4) {
			for (auto b : str) {
				i = (i << 8) | b;
			}
		}
		i = neg.length() == 0 ? i : -i;
		return (NodePtr)std::make_shared<ExpIntegerValue>(i);
	};

	parser["Code"] = [&](const string_t& name, const string_t& pl) {
		auto func = get_function(name);
		if (pl.length() > 0) {
			log.error(position(), "ERROR_CODE_HAS_CODE", name);
		} else if (func && func->args.list.size() > 0) {
			log.warning(position(), "ERROR_CODE_HAS_CODE", name);
		}
		return (NodePtr)std::make_shared<ExpCodeValue>(name);
	};

	parser["ACall"] = [&](const string_t& name, sol::variadic_args args) {
		auto func = get_function(name);
		auto current = functions.back();
		if (!func || !current || current->is_const != func->is_const) {
			log.error(position(), "ERROR_CALL_IN_CONSTANT", name);
		}

		auto call = std::make_shared<ExpCall>(name, func->returns);
		
		call->is_action = true;

		for (auto v : args) {
			call->params.push_back(CastNode<ExpNode>(v));
		}

		check_call(func, call);

		return (NodePtr)call;
	};


	parser["Vari"] = [&](const string_t& name, NodePtr index) {
		auto var = get_variable(name);

		check_get(var, true);

		auto index_exp = CastNode<ExpNode>(index);
		auto exp_vari = std::make_shared<ExpVari>(var, index_exp);

		return (NodePtr)exp_vari;
	};


	parser["Var"] = [&](const string_t& name) {
		auto var = get_variable(name);
		check_get(var, false);

		auto exp_var = std::make_shared<ExpVar>(var);

		return (NodePtr)exp_var;
	};

	parser["Neg"] = [&](NodePtr exp_node) {

		auto exp = CastNode<ExpNode>(exp_node);

		auto exp_neg = std::make_shared<ExpNeg>(exp);

		return (NodePtr)exp_neg;
	};


	parser["Binary"] = [&](sol::variadic_args args) {
		if (args.size() == 1) {
			return sol::lua_value(args[0]);
		}

		auto first = CastNode<ExpNode>(args[0]);

		for (size_t i = 1; i < args.size(); i += 2) {
			std::string op = args[i];
			auto second = CastNode<ExpNode>(args[i + 1]);
			auto exp_type = get_two_exp_type(first, op, second);
			first = std::make_shared<ExpBinary>(exp_type, first, op, second);
		}

		return sol::lua_value((NodePtr)first);
	};

	parser["Unary"] = [&](sol::variadic_args args) {
		if (args.size() == 1) {
			return sol::lua_value(args[0]);
		}
		auto first = CastNode<ExpNode>(args[0]);

		for (size_t i = 1; i < args.size(); i += 2) {
			std::string op = args[i];
			string_t exp_type = "boolean";
			if (op == "not" && first->vtype != "boolean") {
				log.error(position(), "ERROR_NOT_TYPE");
			}
			first = std::make_shared<ExpUnary>(exp_type, first, op);
		}

		return sol::lua_value((NodePtr)first);
	};

	parser["Type"] = [&](const string_t& name, const string_t& extends) {
		check_type(extends);
		check_name(name);
		check_new_name(name);

		auto type = std::make_shared<TypeNode>(name, types.find(extends), file, linecount);

		types.save(name, type);

		return type;
	};

	size_t globals_start_line = 0;

	parser["GlobalsStart"] = [&]() {
		globals_start_line = linecount;
	};

	parser["GlobalsEnd"] = [&](const string_t& str) {
		if (str.length() == 0) {
			log.error(position(), "ERROR_ENDGLOBALS", globals_start_line);
		}
	};

	parser["Globals"] = [&]() {
		
		auto gs = std::make_shared<GlobalsNode>();
		gs->globals = &globals;

		return (NodePtr)gs;
	};

	parser["Global"] = [&](const string_t& constant, const string_t& type, const string_t& array, const string_t& name, sol::object lexp) {
		NodePtr exp;
		if (lexp.get_type() == sol::type::userdata) {
			exp = lexp.as<NodePtr>();
		}
		check_name(name);
		check_new_name(name);
		check_type(type); 

		bool is_const = !constant.empty();

		if (is_const && !exp) {
			log.error(position(), "ERROR_CONSTANT_INIT");
		}

		bool is_array = !array.empty();
		if (is_array) {
			if (exp) {
				log.error(position(), "ERROR_ARRAY_INIT");
			}
			if (type == "code") {
				log.error(position(), "ERROR_CODE_ARRAY");
			}
		}

		if (exp) {
			if (exp->type == "exp_call") {
				auto call = CastNode<ExpCall>(exp);

				switch (hash_s(call->name))
				{
				case "OrderId"s_hash:
				case "OrderId2String"s_hash:
				case "UnitId2String"s_hash:
					//这几个会返回null 
					log.warning(position(), "WARNING_NULL_NATIVE_IN_GLOBAL", call->name);
					break;
				case "GetObjectName"s_hash:
				case "CreateQuest"s_hash:
				case "CreateMultiboard"s_hash:
				case "CreateLeaderboard"s_hash:
					//这几个会崩溃
					log.warning(position(), "WARNING_CRASH_NATIVE_IN_GLOBAL", call->name);
					break;
				default:
					break;
				}
			}
		}

		auto global = std::make_shared<GlobalNode>(is_const, type, name, is_array, file, linecount);

		global->exp = CastNode<ExpNode>(exp);

		globals.save(name, global);

		//return (NodePtr)global;
	};


	parser["LocalDef"] = [&](const string_t& type, const string_t& array, const string_t& name) {

		bool is_array = !array.empty();

		check_name(name);
		check_new_name_functions(name);
		check_new_name_types(name);
		check_type(type);

		check_local_with_args(name, type, is_array);
		check_local_with_globals(name, type, is_array);

		if (is_array) {
			if (type == "code") {
				log.error(position(), "ERROR_CODE_ARRAY");
			}
		}

		auto local = std::make_shared<LocalNode>(type, name, is_array, file, linecount);

		auto func = functions.back();

		func->locals.save(name, local);
		
		auto global = globals.find(name);
		if (global) { //如果局部变量与全局变量同名 可以影响全局变量
			global->vtype = type;
			global->is_array = is_array;
		}
		
		//return (NodePtr)local;
	};


	

	parser["Local"] = [&](NodePtr loc, NodePtr init_exp){

		auto local = CastNode<LocalNode>(loc);
		auto exp = CastNode<ExpNode>(init_exp);

		if (local && exp) {
			local->has_set = true;

			local->exp = exp;

			if (local->is_array) {
				log.error(position(), "ERROR_ARRAY_INIT");
			}

			if (!is_extends(exp->vtype, local->vtype)) {
				log.error(position(), "ERROR_SET_TYPE", local->name, local->vtype, exp->vtype);
			} 
			auto func = functions.back();
			
			if (func && exp->type == "exp_call") { // 局部变量的初始值不能递归自己
				auto call = CastNode<ExpCall>(init_exp);
				if (func->name == call->name) {
					log.error(position(), "ERROR_LOCAL_RECURSION");
				}
			}
		}
		return loc;
	};



	parser["Point"] = [&]() {
		sol::variadic_results res;

		res.push_back(sol::make_object(lua, file));
		res.push_back(sol::make_object(lua, linecount));

		return res;
	};

	parser["Action"] = [](const string_t& file, size_t line, NodePtr action) {

		return action;
	};

	parser["ECall"] = [&](const string_t& name, sol::variadic_args args) {
		auto func = get_function(name);

		auto call = std::make_shared<ExpCall>(func->name, func->returns);

		for (auto v : args) {
			call->params.push_back(CastNode<ExpNode>(v));
		}

		check_call(func, call);

		return (NodePtr)call;
	};

	parser["Set"] = [&](const string_t& name, sol::variadic_args args) {
		auto var = get_variable(name);
		if (args.size() == 1) {
			auto exp = CastNode<ExpNode>(args[0]);
			check_set(var, false, nullptr, exp);
			var->has_set = true;

			auto set = std::make_shared<ActionSet>(name, exp);
			
			return (NodePtr)set;

		} else {
			auto index = CastNode<ExpNode>(args[0]);
			auto exp = CastNode<ExpNode>(args[1]);
			check_set(var, true, index, exp);

			auto setindex = std::make_shared<ActionSetIndex>(name, index, exp);

			return (NodePtr)setindex;
		}
	};

	parser["Return"] = [&]() {
		auto func = functions.back();

		if (func && func->returns != "nothing") {
			log.error(position(), "ERROR_MISS_RETURN", func->name, func->returns);
		}

		auto ret = std::make_shared<ActionReturn>(nullptr);

		return (NodePtr)ret;
	};


	parser["ReturnExp"] = [&](NodePtr exp_) {

		auto exp = CastNode<ExpNode>(exp_);
		
		auto func = functions.back();
	
		if (func && exp) {
			auto& t1 = func->returns;
			auto& t2 = func->returns;
	
			if (t1 != "nothing") {
				if (t1 == "Real" && t2 == "integer") {
					log.warning(position(), "ERROR_RETURN_INTEGER_AS_REAL", func->name, t1, t2);
				} else if (is_extends(t2, t1)) {
					//rb warning 
				} else {
					log.error(position(), "ERROR_MISS_RETURN", func->name, func->returns);
				}
			}
			
			if (func->is_const && exp->type == "exp_var" && !CastNode<VarBaseNode>(exp)->has_set) {
				log.warning(position(), "ERROR_CONSTANT_UNINIT", func->name);
			}
		}
		
		auto ret = std::make_shared<ActionReturn>(exp);

		return (NodePtr)ret;
	};

	parser["Exit"] = [&](NodePtr exp) {
		if (loop_stack_count == 0) {
			log.error(position(), "ERROR_EXITWHEN");
		}
		auto exit = std::make_shared<ActionExit>(CastNode<ExpNode>(exp));
		return (NodePtr)exit;
	};

	parser["Logic"] = [&](sol::lua_table ifelse, const string_t& endif) {
		if (endif.empty()) {
			auto if_node = CastNode<IfNode>(ifelse[0]);
			log.error(position(), "ERROR_ENDIF", if_node->file, if_node->line);
		}

		auto action_if = std::make_shared<ActionIf>();

		action_if->if_nodes.resize(ifelse.size());

		bool has_return = true;
		for (size_t i = 0; i < ifelse.size(); i++) {  //三个分支必须都要有返回值
			auto node = CastNode<IfNode>(ifelse[i]);
			action_if->if_nodes[i] = node;
			if (!node->has_return) {
				has_return = false;
			}
		}
		action_if->has_return = has_return;

		return (NodePtr)action_if;
	};


	parser["IfStart"] = [&]() {
		sol::variadic_results res;

		res.push_back(sol::make_object(lua, file));
		res.push_back(sol::make_object(lua, linecount));

		return res;
	};

	parser["If"] = [&](const string_t& file, size_t line, NodePtr condition, sol::variadic_args args) {
		auto exp = CastNode<ExpNode>(condition);
		if (exp->vtype != "boolean") {
			log.warning(position(), "ERROR_CONDITION_TYPE");
		}

		auto if_node = std::make_shared<IfNode>(IfNode::TYPE::IF, file, line);
		if_node->condition = exp;
		if_node->actions.resize(args.size());
		size_t i = 0;
		for (auto v : args) {
			auto action = CastNode<ActionNode>(v);
			if_node->actions[i++] = action;

			if (action->has_return) {
				if_node->has_return = true;
			}
		}

		return (NodePtr)if_node;
	};


	parser["ElseifStart"] = parser["IfStart"];


	parser["Elseif"] = [&](const string_t& file, size_t line, NodePtr condition, sol::variadic_args args) {
		auto exp = CastNode<ExpNode>(condition);
		if (exp->vtype != "boolean") {
			log.error(position(), "ERROR_CONDITION_TYPE");
		}

		auto elseif_node = std::make_shared<IfNode>(IfNode::TYPE::ELSEIF, file, line);
		elseif_node->condition = exp;
		elseif_node->actions.resize(args.size());
		size_t i = 0;
		for (auto v : args) {
			auto action = CastNode<ActionNode>(v);
			elseif_node->actions[i++] = action;
			if (action->has_return) {
				elseif_node->has_return = true;
			}
		}

		return (NodePtr)elseif_node;
	};

	parser["ElseStart"] = parser["IfStart"];

	parser["Else"] = [&](const string_t& file, size_t line, sol::variadic_args args) {
		auto else_node = std::make_shared<IfNode>(IfNode::TYPE::ELSE, file, line);
		else_node->actions.resize(args.size());
		size_t i = 0;
		for (auto v : args) {
			auto action = CastNode<ActionNode>(v);
			else_node->actions[i++] = action;
			if (action->has_return) {
				else_node->has_return = true;
			}
		}
		return (NodePtr)else_node;
	};

	parser["LoopStart"] = [&]() {
		loop_stack_count++;
		return linecount;
	};

	parser["Loop"] = [&](size_t line, sol::lua_table chunks, const string_t& endloop) {
		if (endloop.empty()) {
			log.error(position(), "ERROR_ENDLOOP", line);
		}
		loop_stack_count--;

		auto loop = std::make_shared<ActionLoop>(linecount);

		loop->actions.resize(chunks.size());

		for (size_t i = 0; i < chunks.size(); i++) {
			loop->actions[i] = CastNode<ActionNode>(chunks[i]);
		}
		return (NodePtr)loop;
	};

	
	parser["NArgs"] = [&](sol::object takes) {
		printf("bbbbbbbbbbb\n");
		return takes;
	};

	parser["FArgs"] = [&](sol::object takes) {

		printf("cccccccccccccc\n");
		return takes;
	};

	parser["Native"] = [&](
		const string_t& file,
		size_t line, 
		const string_t& constant,
		const string_t& name,
		sol::object takes_,
		const string_t& returns
	) {
		printf("aaaaaaaaaaaaa\m");
		check_name(name);
		check_new_name(name);
		check_type(returns);

		bool is_const = !constant.empty();

		auto native = std::make_shared<NativeNode>(is_const, name, returns, file, line);

		if (takes_.get_type() == sol::type::table) {
			sol::lua_table takes = takes_.as<sol::lua_table>();

			printf("size %i\n", takes.size());
			
			for (size_t i = 0; i < takes.size(); i++) {
				printf("%i  %i\n", i, takes[i].get_type());
			}

;			for (size_t i = 0; i < takes.size(); i += 2) {
				const string_t type = takes[i].get<string_t>();
				const string_t name = takes[i + 1].get<string_t>();

				auto arg = std::make_shared<ArgNode>(type, name);

				auto var = globals.find(name);
				if (var) {
					log.error(position(), "ERROR_REDEFINE_GLOBAL", name, var->file, var->line);
				}
				if (!types.find(type)) {
					log.error(position(), "ERROR_UNDEFINE_TYPE", type);
				}
				native->args.save(name, arg);
			}
		}
		

		natives.save(name, native);

		return (NodePtr)native;
	};

	//parser["Native"] = [&](sol::variadic_args args) {
	//
	//
	//	printf("xxxxxxxx  %x\n", args.size());
	//	for (auto v : args) {
	//		printf("%i\n", v.get_type());
	//	}
	//};

	parser["FunctionStart"] = [&](const string_t& constant, const string_t& name, sol::lua_table takes, const string_t& returns) {
		check_name(name);
		check_new_name(name);
		check_type(returns);

		bool is_const = !constant.empty();

		auto func = std::make_shared<FunctionNode>(is_const, name, returns, file, linecount);

		for (size_t i = 0; i < takes.size(); i += 2) {
			const string_t& type = takes[i];
			const string_t& name = takes[i + 1];

			auto arg = std::make_shared<ArgNode>(type, name);

			auto var = globals.find(name);
			if (var) {
				log.error(position(), "ERROR_REDEFINE_GLOBAL", name, var->file, var->line);
			}
			if (!types.find(type)) {
				log.error(position(), "ERROR_UNDEFINE_TYPE", type);
			}
			func->args.save(name, arg);
		}

		functions.save(name, func);

		return (NodePtr)func;
	};

	parser["FunctionBody"] = [&](sol::lua_table locals, sol::lua_table actions) {
		auto func = functions.back();

		func->actions.resize(actions.size());

		for (size_t i = 0; i < actions.size(); i++) {
			func->actions[i] = CastNode<ActionNode>(actions[i]);
		}
	};

	parser["FunctionEnd"] = [&](const string_t& endfunction) {
		auto func = functions.back();

		if (endfunction.empty()) {
			log.error(position(), "ERROR_ENDFUNCTION", func->line);
		}
		return (NodePtr)func;
	};


	parser["Chunk"] = [](NodePtr chunk) {
		return chunk;
	};


	parser["returnAsReturns"] = [&]() {
		log.error(position(), "ERROR_RETURN_AS_RETURNS");
	};

	parser["setAsCall"] = [&]() {
		log.error(position(), "ERROR_SET_AS_CALL");
	};

	parser["callAsSet"] = [&]() {
		log.error(position(), "ERROR_CALL_AS_SET");
	};

	parser["constantLocal"] = [&]() {
		log.error(position(), "ERROR_CONSTANT_LOCAL");
	};

	parser["typeInFunction"] = [&]() {
		log.error(position(), "ERROR_TYPE_IN_FUNCTION");
	};

	parser["localInFunction"] = [&]() {
		log.error(position(), "ERROR_LOCAL_IN_FUNCTION");
	};



	parser["errorpos"] = [](int line, int col, string_t at_line, string_t err) {
		std::string msg = std::format("error:{}:{}:   {}:\n{}\n", line, col, err, at_line);
		printf(msg.c_str());
	};

	//peg_parser(script, parser);

	peg_parser(script, parser);
	//std::cout << res->type << std::endl;
	return;
}