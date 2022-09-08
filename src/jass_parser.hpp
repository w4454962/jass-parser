#pragma once
#include "stdafx.h"
#include "jass_peg_rule.h"
#include "mimalloc.h"
std::string_view convert_message(std::string_view msg);

int num = 0;

int create_count = 0;
int delete_count = 0;

typedef std::shared_ptr<struct Node> NodePtr;
typedef std::shared_ptr<struct VarBaseNode> VarPtr;
typedef std::shared_ptr<struct ExpNode> ExpPtr;
typedef std::shared_ptr<struct ActionNode> ActionPtr;

template<typename N, typename T>
inline auto CastNode(const T& v) {
	return std::dynamic_pointer_cast<N>((NodePtr)v);
}

using string_t = std::string_view;



template< typename Type >
struct Container
{
	typedef std::shared_ptr<Type> object_ptr;

	std::vector<object_ptr> list;
	std::unordered_map <string_t, size_t> map;

	bool save(const string_t& name, const object_ptr& obj) {
		if (map.find(name) != map.end()) {
			return false;
		}
		size_t back_index = list.size();
		list.emplace_back(obj);

		using gc_string_t = class gc_string_t;

		map.emplace(gc_string_t(name), back_index);
		return true;
	}


	object_ptr find(const string_t& name) {
		auto it = map.find(name);
		if (it == map.end()) {
			return nullptr;
		}
		return list[it->second];
	}

	object_ptr back() {
		if (list.empty()) {
			return nullptr;
		}
		return list.back();
	}
};

template<typename Key, typename Type>
struct NodeContainer
{
	typedef std::shared_ptr<Type> object_ptr;

	std::unordered_map <Key, object_ptr> map;

	bool save(const Key& key, object_ptr obj) {
		if (map.find(key) != map.end()) {
			return false;
		}
		map.emplace(key, obj);
		return true;
	}

	object_ptr find(const Key& key) {
		auto it = map.find(key);
		if (it == map.end()) {
			return nullptr;
		}
		return it->second;
	}
};

template<typename type, size_t N>
struct Stack
{
	type pool[N];
	size_t size = N;
	size_t pos;

	Stack() {
		pos = -1;
	}

	[[nodiscard]] void emplace_back(const type& v) {
		pool[++pos] = v;
	}


	[[nodiscard]] bool empty() {
		return pos == -1;
	}

	[[nodiscard]] type& back() {
		assert(!empty());
		return pool[pos];
	}

	[[nodiscard]] void pop_back() {
		pool[pos--] = nullptr;
	}
};

struct Cache {
	NodeContainer<string_t, struct ExpStringValue> string_nodes;
	NodeContainer<float, struct ExpRealValue> real_nodes;
	NodeContainer<int, struct ExpIntegerValue> integer_nodes;
	NodeContainer<string_t, struct ExpCodeValue> code_nodes;

	NodeContainer<void*, ExpNode> var_nodes;

	std::unordered_map<std::string_view, std::shared_ptr<std::string>> string_map;


	template<typename T>
	std::string_view string(const T& s) {
		std::string_view result;

		auto it = string_map.find(s);
		if (it != string_map.end()) {
			result = *it->second;
		} else {
			//std::cout << "cache {" << s << "}\n";
			auto ptr = std::make_shared<std::string>(s);
			result = *ptr;
			string_map.emplace(result, ptr);
		}
		return result;
	}
};

std::shared_ptr<struct Cache> jass_gc;



class gc_string_t : public std::string_view {
public:
	gc_string_t() 
		:std::string_view()
	{ }
	gc_string_t(const char* source)
		: std::string_view(jass_gc->string(source))
	{ }

	gc_string_t(const std::string& source) 
		: std::string_view(jass_gc->string(source))
	{ }

	gc_string_t(const std::string_view& source)
		: std::string_view(jass_gc->string(source))
	{ }

};




enum class NodeType
{
	NONE,
	VAR,
	EXP,
	EXP_VALUE,
	EXP_VAR,
	EXP_VARI,
	EXP_NEG,
	EXP_BINRAY,
	EXP_UNARY,
	EXP_CALL,
	ACTION,
	ACTION_CALL,
	ACTION_SET,
	ACTION_SETINDEX,
	ACTION_RETURN,
	ACTION_EXITWHEN,
	IF,
	ACTION_IF,
	ACTION_LOOP,
	TYPE,
	VAR_ARG,
	VAR_LOCAL,
	VAR_GLOBAL,
	GLOBALS,
	NATIVE,
	FUNCTION,
	JASS,
};

typedef std::function<bool(NodePtr node, size_t branch_index)> NodeFilter;


struct Node {
	NodeType type = NodeType::NONE;

	virtual const NodeType get_type (){  return type; }
	virtual bool each_childs(const NodeFilter& filter) { return 0; }
	virtual size_t get_childs_size() { return 0; }
	virtual bool add_child(const NodePtr& node) { return 0; }
};



struct VarBaseNode : Node {
	gc_string_t vtype;
	gc_string_t name;

	bool has_set;

	VarBaseNode() {
		type = NodeType::VAR;
		has_set = false;
	}
};


struct ExpNode: Node {
	gc_string_t vtype = "nothing";

	ExpNode() {
		type = NodeType::EXP;
	}
};

struct ExpValue : ExpNode {

	ExpValue() {
		type = NodeType::EXP_VALUE;
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
	gc_string_t value;
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
	gc_string_t value;
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
		type = NodeType::EXP_VAR;
		vtype = var->vtype;
	}
};

struct ExpVari : ExpNode {
	VarPtr var;

	ExpPtr index;

	ExpVari(VarPtr& var_, ExpPtr& index_)
		:var(var_),
		index(index_)
	{
		type = NodeType::EXP_VARI;
		vtype = var->vtype;
	}
};

struct ExpNeg : ExpNode {
	ExpPtr exp;

	ExpNeg(ExpPtr& exp_)
		: exp(exp_)
	{
		type = NodeType::EXP_NEG;
		vtype = exp_->vtype;;
	}
};


struct ExpBinary : ExpNode {

	gc_string_t op;

	ExpPtr first;
	ExpPtr second;

	ExpBinary() {
		type = NodeType::EXP_BINRAY;
	}

	ExpBinary(const string_t& vtype_, ExpPtr& first_, const string_t& op_, ExpPtr& second_)
		: op(op_),
		first(first_),
		second(second_)
	{
		type = NodeType::EXP_BINRAY;
		vtype = vtype_;
	}
};

struct ExpUnary :ExpNode {
	gc_string_t op;
	ExpPtr first;

	ExpUnary(const string_t& vtype_, ExpPtr& first_, const string_t& op_)
		: op(op_),
		first(first_)
	{
		type = NodeType::EXP_UNARY;
		vtype = vtype_;
	}
};


struct ExpCall : ExpNode {
	gc_string_t name;
	std::vector<ExpPtr> params;

	bool is_action;

	ExpCall(const string_t& func_name_, const string_t& func_return_type)
		: name(func_name_)
	{
		type = NodeType::EXP_CALL;
		vtype = func_return_type;
		is_action = false;
	}
};




struct ActionNode :Node {
	bool has_return;
	
	ActionNode() {
		type = NodeType::ACTION;
		has_return = false;
	}

};

struct ActionCall :ActionNode {
	std::shared_ptr<ExpCall> call;

	ActionCall(std::shared_ptr<ExpCall>& call_)
		:call(call_)
	{
		type = NodeType::ACTION_CALL;
	}
};

struct ActionSet : ActionNode {
	gc_string_t name;

	ExpPtr exp;

	ActionSet(const string_t& name_, ExpPtr& exp_)
		: name(name_),
		exp(exp_)
	{
		type = NodeType::ACTION_SET;
	}
};


struct ActionSetIndex : ActionNode {
	gc_string_t name;
	ExpPtr index;
	ExpPtr exp;

	ActionSetIndex(const string_t& name_, ExpPtr& index_, ExpPtr& exp_)
		: name(name_),
		index(index_),
		exp(exp_)
	{
		type = NodeType::ACTION_SETINDEX;
	}
};

struct ActionReturn : ActionNode {
	ExpPtr value;

	ActionReturn(ExpPtr exp_) 
		:value(exp_)
	{
		type = NodeType::ACTION_RETURN;
		has_return = true;
	}
};

struct ActionExit : ActionNode {
	ExpPtr exp;

	ActionExit(ExpPtr exp_)
		:exp(exp_)
	{
		type = NodeType::ACTION_EXITWHEN;
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
		type = NodeType::IF;
	}

	virtual bool each_childs(const NodeFilter& filter) override {
		for (auto& action : actions) {
			if (filter(action, (size_t)iftype))
				return 1;
		}

		return 0;
	}

	virtual size_t get_childs_size() override {
		return actions.size();
	}

	virtual bool add_child(const NodePtr& node) override {
		actions.emplace_back(CastNode<ActionNode>(node));
		return 1;
	}
};

struct ActionIf : ActionNode {
	std::vector<std::shared_ptr<IfNode>> if_nodes;

	ActionIf() {
		type = NodeType::ACTION_IF;
	}

	virtual bool each_childs(const NodeFilter& filter) override {
		for (auto& node : if_nodes) {
			if (filter(node, (size_t)node->iftype)) {
				return 1;
			}
		}
		return 0;
	}

	virtual size_t get_childs_size() override {
		size_t size = 0;

		for (auto& node : if_nodes) {
			size += node->actions.size();
		}
		return size; 
	}

	virtual bool add_child(const NodePtr& node) override {
		if_nodes.emplace_back(CastNode<IfNode>(node));
		return 1; 
	}

};

struct ActionLoop :ActionNode {
	std::vector<ActionPtr> actions;

	size_t endline;

	ActionLoop(size_t endline_)
		:endline(endline_)
	{
		type = NodeType::ACTION_LOOP;
	}

	virtual bool each_childs(const NodeFilter& filter) override {
		size_t brnch_index = 0;
		for (auto& action : actions) {
			if (filter(action, brnch_index))
				return 1;
		}
		return 0;
	}

	virtual size_t get_childs_size() override {
		return actions.size();
	}

	virtual bool add_child(const NodePtr& node) override {
		actions.emplace_back(CastNode<ActionNode>(node));
		return 1;
	}
};



struct TypeNode : Node {
	gc_string_t file;
	size_t line;
	gc_string_t name;
	std::shared_ptr<TypeNode> parent;
	std::set<gc_string_t> childs;

	TypeNode(const string_t& name_, std::shared_ptr<TypeNode> parent_, const string_t& file_, size_t line_)
		: name(name_),
		parent(parent_),
		file(file_),
		line(line_)
	{ 
		type = NodeType::TYPE;
	}
};



struct ArgNode : VarBaseNode {

	ArgNode(const string_t& type_, const string_t& name_)
	{ 
		vtype = type_;
		name = name_;
		type = NodeType::VAR_ARG;
		has_set = true;
	}
};

struct LocalNode : VarBaseNode {
	gc_string_t file;
	size_t line;
	
	bool is_array;

	ExpPtr exp;

	LocalNode(const string_t& type_, const string_t& name_, bool is_array_, const string_t& file_, size_t line_)
		: is_array(is_array_),
		file(file_),
		line(line_)
	{
		vtype = type_;
		name = name_;
		type = NodeType::VAR_LOCAL;
	}
};

struct GlobalNode : LocalNode {

	bool is_const;

	GlobalNode(bool constant, const string_t& type_, const string_t& name_, bool is_array_, const string_t& file_, size_t line_)
		: LocalNode(type_, name_, is_array_, file_, line_)
	{
		type = NodeType::VAR_GLOBAL;
		is_const = constant;
	}
};

struct GlobalsNode : Node {

	Container<GlobalNode>* globals;

	GlobalsNode() {
		type = NodeType::GLOBALS;
		globals = nullptr;
	}

	virtual bool each_childs(const NodeFilter& filter) override {
		if (!globals) 
			return 0;

		size_t brnch_index = 0;
		for (auto& global : globals->list) {
			if (filter(global, brnch_index))  
				return 1;
		}

		return 0;
	}

	virtual size_t get_childs_size() override {
		if (!globals) return 0;
		return globals->list.size();
	}
};


struct NativeNode : Node {
	gc_string_t file;
	size_t line;
	bool is_const;
	gc_string_t name;
	gc_string_t returns;
	Container<ArgNode> args;

	NativeNode(bool is_const_, const string_t& name_, const string_t& returns_, const string_t& file_, size_t line_)
		: is_const(is_const_),
		name(name_),
		returns(returns_),
		file(file_),
		line(line_)
	{
		type = NodeType::NATIVE;
	}
};

struct FunctionNode : NativeNode {
	Container<LocalNode> locals;

	std::vector<ActionPtr> actions;

	bool has_return_any;

	FunctionNode(bool is_const_, const string_t& name_, const string_t& returns_, const string_t& file_, size_t line_)
		: NativeNode(is_const_, name_, returns_, file_, line_)
	{
		type = NodeType::FUNCTION;
		has_return_any = false;
	}

	virtual bool each_childs(const NodeFilter& filter) override {
		size_t brnch_index = 0;
		for (auto& local : locals.list) {
			if (filter(local, brnch_index))  
				return 1;
		}

		for (auto& action : actions) {
			if (filter(action, brnch_index))  
				return 1;
		}

		return 0;
	}

	virtual size_t get_childs_size() override {
		return locals.list.size() + actions.size();
	}


	virtual bool add_child(const NodePtr& node) override {
		actions.emplace_back(CastNode<ActionNode>(node));
		return 1;
	}

};


struct Jass : Node {

	std::shared_ptr<Cache> gc;

	gc_string_t file;

	Container<TypeNode> types;
	Container<GlobalNode> globals;
	Container<NativeNode> natives;
	Container<FunctionNode> functions;

	std::unordered_map<string_t, std::shared_ptr<LocalNode>> exploits;

	Jass(const string_t& file_, std::shared_ptr<Cache>& gc_)
		: gc(gc_),
		file(file_)
	{
		type = NodeType::JASS;


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
	std::string file;
	std::string message;
	size_t line;
	size_t column;
	std::string at_line;
};


typedef std::shared_ptr<ParseErrorMessage> MsgPtr;
struct ParseLog {
	std::vector<MsgPtr> errors;
	std::vector<MsgPtr> warnings;

	template<typename... Args>
	void error(MsgPtr msg, const std::string_view& fmt, Args... args) {
		try {
			msg->message = /*fmt + ":" + */std::vformat(convert_message(fmt), std::make_format_args(args...));
			msg->level = ErrorLevel::error;
			errors.emplace_back(msg);
			std::cout << msg->message << std::endl;
		} catch(...) {
			std::cout << "Crash " << fmt << std::endl;
		}
	}

	template<typename... Args>
	void error_append(const std::string& fmt, Args... args)
	{
		if (errors.size() == 0)
			return;

		auto msg = errors.back();
		msg->message+= std::vformat(convert_message(fmt), std::make_format_args(args...));
	}


	template<typename... Args>
	void warning(MsgPtr msg, const std::string_view& fmt, Args... args) {
		msg->message = std::vformat(convert_message(fmt), std::make_format_args(args...));
		msg->level = ErrorLevel::warning;
		warnings.emplace_back(msg);
		std::cout << msg->message << std::endl;
	}
};


struct ParseConfig {
	std::string file;
	std::shared_ptr<std::string> script;

	bool rb;		//双返回值漏洞
	bool exploit;	//局部变量修改全局变量漏洞

	ParseConfig() {
		rb = true;
		exploit = true;
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


bool jass_parser(sol::state& lua, const ParseConfig& config, ParseResult& result) {

	clock_t start = clock();

	sol::function peg_parser = lua.require_file("peg", "peg");

	sol::table relabel = lua.require_file("relabel", "relabel");

	sol::table parser = lua.create_table();
	
	size_t linecount = 1, column = 0, loop_stack_count = 0;

	const string_t& file = config.file;

	if (!result.jass) {
		jass_gc = std::make_shared<Cache>();
		result.jass = std::make_shared<Jass>(file, jass_gc);
	}

	auto& comments = result.comments;
	auto& functions = result.jass->functions;
	auto& types = result.jass->types;
	auto& globals = result.jass->globals;
	auto& natives = result.jass->natives;
	auto& exploits = result.jass->exploits;
	auto& log = result.log;

	bool has_function = false; 

	Stack<NodePtr, 0x100> block_stack;


	auto position = [&]() {
		auto msg = std::make_shared<ParseErrorMessage>();
		msg->file = file;
		msg->line = linecount;
		msg->column = -1;
		msg->at_line = relabel["line"](*config.script, linecount);

		return msg;
	};

	parser["errorpos"] = [&](int line, int col, string_t at_line, string_t err) {

		auto msg = std::make_shared<ParseErrorMessage>();
		msg->file = file;
		msg->line = line;
		msg->column = col;
		msg->at_line = at_line;

		log.error(msg, err);
	};

	auto append_expolit_warning = [&](VarPtr var) {
		if (!config.exploit)
			return;

		if (!var || var->type == NodeType::VAR_ARG)
			return ;
		auto local = CastNode<LocalNode>(var);

		auto& name = var->name;

		auto it = exploits.find(name);
		if (it == exploits.end() || it->second != local) {
			return;
		}

		auto func = functions.back();
		if (func && func->locals.find(name)) {
			return;
		}
		log.error_append("WARNING_REDEFINE_VAR", name, local->file, local->line);
	};

	auto check_name = [&](const string_t& name) {
		if (keywords.find(name) != keywords.end()) {
			log.error(position(), "ERROR_KEY_WORD", name);
		}
	};

	auto check_new_name_types = [&](const string_t& name) {
		auto type = types.find(name);
		if (type) {
			if (type->parent) {
				log.error(position(), "ERROR_REDEFINE_TYPE", name, type->file, type->line);
			} else {
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

	auto is_extends = [&](const gc_string_t& type_name, const gc_string_t& parent_name) {
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
							miss += ", ";
						miss += std::format("{} {}", arg->vtype, arg->name);
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
					if (!is_extends(exp->vtype, arg->vtype)) {
						log.error(position(), "ERROR_WRONG_ARG", func->name, i + 1, arg->vtype, exp->vtype);
						if (exp->type == NodeType::EXP_VAR) {
							auto exp_var = CastNode<ExpVar>(exp);
							append_expolit_warning(exp_var->var);
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

		auto global = globals.find(name);
		if (global) {

			if (config.exploit) {
				auto it = exploits.find(name);
				if (it != exploits.end()) {
					return (VarPtr)it->second;
				}
			}

			return (VarPtr)global;
		}
		log.error(position(), "VAR_NO_EXISTS", name);

		return VarPtr();
	};


	auto check_set = [&](VarPtr& var, bool need_array, ExpPtr index, ExpPtr exp) {
		if (!var) return;
		auto& name = var->name;
		var->has_set = true;

		if (need_array) {
			if (var->type == NodeType::VAR_ARG || !CastNode<LocalNode>(var)->is_array) {
				log.error(position(), "ERROR_WASTE_INDEX", name);
				append_expolit_warning(var);
			}
		} else {
			if (var->type != NodeType::VAR_ARG && CastNode<LocalNode>(var)->is_array) {
				log.error(position(), "ERROR_NO_INDEX", name);
				append_expolit_warning(var);
			}
		}

		if (index && !is_extends(index->vtype, "integer")) {
			log.error(position(), "ERROR_INDEX_TYPE", name, index->vtype);
			append_expolit_warning(var);
		}

		if (has_function) {
			auto func = functions.back();
			if (func && var->type == NodeType::VAR_GLOBAL && CastNode<GlobalNode>(var)->is_const) {
				log.error(position(), "ERROR_SET_CONSTANT", name);
			}
			else if (func && func->is_const && var->type == NodeType::VAR_GLOBAL) {
				log.error(position(), "ERROR_SET_IN_CONSTANT", name);
			}
		}
	

		if (exp && !is_extends(exp->vtype, var->vtype)) {
			log.error(position(), "ERROR_SET_TYPE", name, var->vtype, exp->vtype);
			append_expolit_warning(var);
		}
	};

	auto check_get = [&](VarPtr& var, bool need_array) {
		if (!var) return;

		auto& name = var->name;

		if (need_array) {
			if (var->type == NodeType::VAR_ARG || !CastNode<LocalNode>(var)->is_array ) {
				log.error(position(), "ERROR_WASTE_INDEX", name);
				append_expolit_warning(var);
			}
		} else {
			if (var->type != NodeType::VAR_ARG && CastNode<LocalNode>(var)->is_array) {
				log.error(position(), "ERROR_NO_INDEX", name);
				append_expolit_warning(var);
			}
			if (!var->has_set) {
				//log.warning(position(), "ERROR_GET_UNINIT", name);
				//append_expolit_warning(var);
			}
		}
	};

	auto check_local_with_args = [&](const string_t& name, const string_t& type, bool is_array) {
		auto func = functions.back();
		if (!func) return;
		auto var = func->args.find(name);
		if (!var) return; 

		if (is_array) {
			log.error(position(), "ERROR_REDEFINE_ARRAY_WITH_ARG", name, func->name, func->file, func->line);
			return;
		}

		if (type != var->vtype) {
			log.error(position(), "ERROR_REDEFINE_VAR_TYPE_WITH_ARG", name, type, func->name, var->vtype, func->file, func->line);
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

	auto get_binary_exp_type = [&](ExpPtr& first, const string_t& op, ExpPtr& second) {
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
		case '=':
			log.error(position(), "ERROR_ASSIGN_AS_EQ");
			break;
		default:
			log.error(position(), "UNKNOWN_EXP");
			break;
		}
		return exp_type;
	};

	auto cast_integer = [&](const std::string& str, int flag) {
		int value;
		try{
			value = std::stoul(str, 0, flag);
		} catch(...) {
			log.error(position(), "WARNING_INTEGER_OVERFLOW", str);
		}

		auto node = jass_gc->integer_nodes.find(value);
		if (node) {
			return (NodePtr)node;
		}
		node = std::make_shared<ExpIntegerValue>(value);
		jass_gc->integer_nodes.save(value, node);

		return (NodePtr)node;
	};


	auto block_add_node = [&](const NodePtr& node) {
		if (block_stack.empty()) {
			return;
		}

		auto& back = block_stack.back();
		//std::cout << "add child [" << back->type << "] << " << node->type << "\n";
		back->add_child(node);
	};

	auto lua_nil_table = lua.create_table();
	{
		auto mt = lua.create_table();
		lua_nil_table[sol::metatable_key] = mt;

		mt["__tostring"] = []() {
			return "<null>";
		};
	}
	

	parser["Nil"] = [&]() {
		return lua_nil_table;
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

	NodePtr null_value(std::make_shared<ExpNullValue>());
	
	parser["NULL"] = [&] {
		return null_value;
	};

	NodePtr true_value(std::make_shared<ExpBooleanValue>(true));
	
	parser["TRUE"] = [&] {
		return true_value;
	};

	NodePtr false_value(std::make_shared<ExpBooleanValue>(false));

	parser["FALSE"] = [&] {
		return false_value;
	};

	parser["String"] = [](const string_t& str) {
		auto node = jass_gc->string_nodes.find(str);
		if (node) {
			return (NodePtr)node;
		}
		node = std::make_shared<ExpStringValue>(str);
		jass_gc->string_nodes.save(str, node);

		return (NodePtr)node;
	};
	
	parser["Real"] = [](const std::string& neg, const std::string& str) {

		float value = 0.f;
		try {
			value = std::stof(neg + str);
		} catch (...) { 

		}
		auto node = jass_gc->real_nodes.find(value);
		if (node) {
			return (NodePtr)node;
		}
		node = std::make_shared<ExpRealValue>(value);
		jass_gc->real_nodes.save(value, node);

		return (NodePtr)node;
	};

	parser["Integer8"] = [&](const std::string& neg, const std::string& str) {
		return cast_integer(neg + str, 8);
	};

	parser["Integer10"] = [&](const std::string& neg, const std::string& str) {
		return cast_integer(neg + str, 10);
	};

	parser["Integer16"] = [&](const std::string& neg, const std::string& str) {
		return cast_integer(neg + str, 16);
	};

	parser["Integer256"] = [&](const string_t& neg, const string_t& str) {
		int i = 0;
		if (str.length() == 1) {
			i = str[0];
		} else if (str.length() == 4) {
			for (auto b : str) {
				if (b == '\\') {
					log.error(position(), "ERROR_INT256_ESC");
					break;
				}
				i = (i << 8) | b;
			}
		}
		i = neg.length() == 0 ? i : -i;

		auto node = jass_gc->integer_nodes.find(i);
		if (node) {
			return (NodePtr)node;
		}
		node = std::make_shared<ExpIntegerValue>(i);
		jass_gc->integer_nodes.save(i, node);
		return (NodePtr)node;
	};

	parser["Code"] = [&](const string_t& name, std::optional<string_t> pl) {
		auto func = get_function(name);

		if (pl.has_value() && !pl.value().empty()) {
			log.error(position(), "ERROR_CODE_HAS_CODE", name);
		} else if (func && func->args.list.size() > 0) {
			log.warning(position(), "ERROR_CODE_HAS_CODE", name);
		}


		auto node = jass_gc->code_nodes.find(name);
		if (node) {
			return (NodePtr)node;
		}
		node = std::make_shared<ExpCodeValue>(name);
		jass_gc->code_nodes.save(name, node);

		return (NodePtr)node;
	};

	parser["ECall"] = [&](const string_t& name, sol::variadic_args args) {
		auto func = get_function(name);

		if (!func) {
			return NodePtr();
		}
		auto call = std::make_shared<ExpCall>(func->name, func->returns);

		call->params.resize(args.size());
		size_t i = 0;
		for (auto v : args) {
			call->params[i++] = CastNode<ExpNode>(v);
		}

		check_call(func, call);

		return (NodePtr)call;
	};



	parser["Vari"] = [&](const string_t& name, NodePtr index) {
		auto var = get_variable(name);

		check_get(var, true);

		if (!var) {
			return NodePtr();
		}
		auto index_exp = CastNode<ExpNode>(index);
		auto exp_vari = std::make_shared<ExpVari>(var, index_exp);

		return (NodePtr)exp_vari;
	};


	parser["Var"] = [&](const string_t& name) {
		auto var = get_variable(name);
		
		check_get(var, false);
		if (!var) {
			return NodePtr();
		}

		auto exp_var = jass_gc->var_nodes.find(var.get());
		if (!exp_var) {
			exp_var = std::make_shared<ExpVar>(var);
			jass_gc->var_nodes.save(var.get(), exp_var);
		}

		return (NodePtr)exp_var;
	};

	parser["Neg"] = [&](NodePtr exp_node) {

		auto exp = CastNode<ExpNode>(exp_node);

		if (exp->vtype != "real" && exp->vtype != "integer") {
			log.error(position(), "ERROR_NEG", exp->vtype);
		}

		auto exp_neg = std::make_shared<ExpNeg>(exp);

		return (NodePtr)exp_neg;
	};

	
	parser["Binary"] = [&](sol::variadic_args args) {
		if (args.size() == 1) {
			return sol::lua_value(args[0]);
		}

		auto first = CastNode<ExpNode>(args[0]);

		size_t p = 0;
		for (size_t i = 1; i < args.size(); i += 2) {
			const std::string& op = args[i];
			auto second = CastNode<ExpNode>(args[i + 1]);
			auto exp_type = get_binary_exp_type(first, op, second);
			first = std::make_shared<ExpBinary>(exp_type, first, op, second);
		}

		return sol::lua_value((NodePtr)first);
	};

	parser["Unary"] = [&](sol::variadic_args args) {
		if (args.size() == 1) {
			return sol::lua_value(args[0]);
		}
		size_t backend = args.size() - 1;

		auto first = CastNode<ExpNode>(args[backend]);

		for (size_t i = backend - 1; i > 0; i--) {
			const std::string& op = args[i];
			string_t exp_type = "boolean";
			if (op == "not" && first->vtype != "boolean") {
				log.error(position(), "ERROR_NOT_TYPE");
			}
			first = std::make_shared<ExpUnary>(exp_type, first, op);
			num++;
		}

		return sol::lua_value((NodePtr)first);
	};

	parser["Type"] = [&](const string_t& name, const string_t& extends) {
		check_type(extends);
		check_name(name);
		check_new_name(name);

		auto parent = types.find(extends);
		auto type = std::make_shared<TypeNode>(name, parent, file, linecount);

		types.save(name, type);

		if (parent) { //继承置空能力
			if (auto it = parent->childs.find("null"); it != parent->childs.end()) {
				type->childs.emplace("null");
			}

			while (parent) {
				parent->childs.emplace(name);
				parent = parent->parent;
			}
		}
		return (NodePtr)type;
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

	parser["Global"] = [&](const string_t& constant, const string_t& type, const string_t& array, const string_t& name, std::optional<NodePtr> lexp) {
		NodePtr exp;

		if (lexp.has_value()) {
			exp = lexp.value();
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

		if (exp && exp->type == NodeType::EXP_CALL) {

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

		auto global = std::make_shared<GlobalNode>(is_const, type, name, is_array, file, linecount);

		global->exp = CastNode<ExpNode>(exp);

		if (global->exp) {
			VarPtr var = global;
			check_set(var, is_array, ExpPtr(), global->exp);
		}

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

		if (config.exploit) {
			auto global = globals.find(name);
			if (global && config.exploit) { //如果局部变量与全局变量同名 可以影响全局变量
				global->vtype = type;
				global->is_array = is_array;

				exploits.emplace(gc_string_t(name), local);
			}
		}
		
		
		return (NodePtr)local;
	};


	parser["Local"] = [&](sol::object loc, std::optional<NodePtr> exp_){

		auto local = CastNode<LocalNode>(loc.as<NodePtr>());
		ExpPtr exp;

		if (exp_.has_value()) {
			exp = CastNode<ExpNode>(exp_.value());
		}

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
			
			if (func && exp->type == NodeType::EXP_CALL) { // 局部变量的初始值不能递归自己
				auto call = CastNode<ExpCall>(exp);
				if (func->name == call->name) {
					log.error(position(), "ERROR_LOCAL_RECURSION");
				}
			}
		}

		if (local) {
			auto func = functions.back();

			func->locals.save(local->name, local);
		}
		

		//return loc;
	};



	parser["ACall"] = [&](const string_t& name, sol::variadic_args args) {
		auto func = get_function(name);
		auto current = functions.back();

		if (!func && current && current->is_const && !func->is_const) {
			log.error(position(), "ERROR_CALL_IN_CONSTANT", name);
		}

		string_t exp_type = "nothing";
		if (func) {
			exp_type = func->returns;
		}

		auto call = std::make_shared<ExpCall>(name, exp_type);

		call->is_action = true;
		call->params.resize(args.size());
		size_t i = 0;
		for (auto v : args) {
			call->params[i++] = CastNode<ExpNode>(v);
		}

		if (func) {
			check_call(func, call);
		}

		auto action = std::make_shared<ActionCall>(call);

		block_add_node(action);

		//return(NodePtr)action;
	};

	parser["Set"] = [&](const string_t& name, sol::variadic_args args) {
		auto var = get_variable(name);

		if (!var) {
			return; //NodePtr();
		}

		if (args.size() == 1) {
			auto exp = CastNode<ExpNode>(args[0]);
			check_set(var, false, nullptr, exp);
			var->has_set = true;

			auto set = std::make_shared<ActionSet>(name, exp);
			
			block_add_node(set);

			//return(NodePtr)set;

		} else {
			auto index = CastNode<ExpNode>(args[0]);
			auto exp = CastNode<ExpNode>(args[1]);
			check_set(var, true, index, exp);

			auto setindex = std::make_shared<ActionSetIndex>(name, index, exp);

			block_add_node(setindex);

			//return(NodePtr)setindex;
		}
	};

	parser["Return"] = [&]() {
		auto func = functions.back();

		if (func && func->returns != "nothing") {
			log.error(position(), "ERROR_MISS_RETURN", func->name, func->returns);
		}

		auto ret = std::make_shared<ActionReturn>(nullptr);

		func->has_return_any = true;

		block_add_node(ret);
		//return (NodePtr)ret;
	};


	parser["ReturnExp"] = [&](NodePtr exp_) {

		auto exp = CastNode<ExpNode>(exp_);
		
		auto func = functions.back();
	
		if (func && exp) {
			auto& t1 = func->returns;
			auto& t2 = exp->vtype;
			VarPtr var;

			if (exp->type == NodeType::EXP_VAR) {
				auto exp_var = CastNode<ExpVar>(exp);
				var = exp_var->var;
			}

			if (t1 != "nothing") {
				if (t1 == "real" && t2 == "integer") {
					log.warning(position(), "ERROR_RETURN_INTEGER_AS_REAL", func->name, t1, t2);
				} else if (!is_extends(t2, t1)) {
					
					if (!config.rb) {
						log.error(position(), "ERROR_RETURN_TYPE", func->name, t2, t1);
						append_expolit_warning(var);
					} else {

					}
				} 
			} else {
				log.error(position(), "ERROR_MISS_RETURN", func->name, t2);
			}
			
			if (func->is_const && exp->type == NodeType::EXP_VAR && !CastNode<ExpVar>(exp)->var->has_set) {
				log.warning(position(), "ERROR_CONSTANT_UNINIT", func->name);
			}

			func->has_return_any = true;
		}
		
		auto ret = std::make_shared<ActionReturn>(exp);

		block_add_node(ret);

		//return (NodePtr)ret;
	};

	parser["Exit"] = [&](NodePtr exp) {
		if (loop_stack_count == 0) {
			log.error(position(), "ERROR_EXITWHEN");
		}
		auto exit = std::make_shared<ActionExit>(CastNode<ExpNode>(exp));

		block_add_node(exit);
		//return (NodePtr)exit;
	};

	
	parser["IfStart"] = [&]() {

		auto action_if = std::make_shared<ActionIf>();

		block_stack.emplace_back(action_if);

		auto if_node = std::make_shared<IfNode>(IfNode::TYPE::IF, file, linecount);

		block_stack.emplace_back(if_node);

	};

	parser["If"] = [&](NodePtr condition) {
		auto exp = CastNode<ExpNode>(condition);
		if (exp->vtype != "boolean") {
			log.warning(position(), "ERROR_CONDITION_TYPE");
		}

		auto if_node = CastNode<IfNode>(block_stack.back());

		if_node->condition = exp;

		for (auto& act : if_node->actions) {
			auto action = CastNode<ActionNode>(act);
			if (action->has_return) {
				if_node->has_return = true;
				break;
			}
		}

		block_stack.pop_back();

		block_add_node(if_node);
	};


	parser["ElseifStart"] = [&]() {

		auto elseif_node = std::make_shared<IfNode>(IfNode::TYPE::ELSEIF, file, linecount);

		block_stack.emplace_back(elseif_node);

	};

	parser["Elseif"] = [&](NodePtr condition) {
		auto exp = CastNode<ExpNode>(condition);
		if (exp->vtype != "boolean") {
			log.error(position(), "ERROR_CONDITION_TYPE");
		}

		auto elseif_node = CastNode<IfNode>(block_stack.back());

		elseif_node->condition = exp;
		
		for (auto& act : elseif_node->actions) {
			auto action = CastNode<ActionNode>(act);
			if (action->has_return) {
				elseif_node->has_return = true;
				break;
			}
		}

		block_stack.pop_back();

		block_add_node(elseif_node);
	};

	parser["ElseStart"] = [&]() {

		auto else_node = std::make_shared<IfNode>(IfNode::TYPE::ELSE, file, linecount);

		block_stack.emplace_back(else_node);
	};

	parser["Else"] = [&]() {
		auto else_node = CastNode<IfNode>(block_stack.back());
		

		for (auto& act : else_node->actions) {
			auto action = CastNode<ActionNode>(act);
			if (action->has_return) {
				else_node->has_return = true;
				break;
			}
		}

		block_stack.pop_back();

		block_add_node(else_node);
	};

	parser["Logic"] = [&](sol::object endif) {

		auto action_if = CastNode<ActionIf>(block_stack.back());

		if (endif.get_type() != sol::type::string) {
			auto if_node = action_if->if_nodes[0];
			log.error(position(), "ERROR_ENDIF", if_node->line);
		}


		bool has_return = true;
		bool has_else = false;

		for (auto& node : action_if->if_nodes) {
			auto ifnode = CastNode<IfNode>(node);
			if (!ifnode->has_return) {
				has_return = false;
			}
			if (ifnode->iftype == IfNode::TYPE::ELSE) {
				has_else = true;
			}
		}


		//每个分支必须都要有返回值 并且最后一个分支是else的时候 才代表整个逻辑有返回值
		action_if->has_return = has_return && has_else;

		block_stack.pop_back();

		block_add_node(action_if);

	};

	parser["LoopStart"] = [&]() {

		auto loop = std::make_shared<ActionLoop>(linecount);

		block_stack.emplace_back(loop);

		loop_stack_count++;
		return linecount;
	};

	parser["Loop"] = [&](size_t line, sol::object endloop) {
		if (endloop.get_type() != sol::type::string) {
			log.error(position(), "ERROR_ENDLOOP", line);
		}
		loop_stack_count--;

		auto loop = CastNode<ActionLoop>(block_stack.back());


		for (auto& act : loop->actions) {
			auto action = CastNode<ActionNode>(act);
			if (action->has_return) {
				loop->has_return = true;
				break;
			}
		}

		block_stack.pop_back();

		block_add_node(loop);
	};

	
	parser["NArgs"] = [&](sol::object takes) {
		return takes;
	};

	parser["FArgs"] = [&](sol::object takes) {
		return takes;
	};

	parser["Native"] = [&](
		std::optional<sol::object> constant,
		const string_t& name,
		std::optional<sol::object> takes_,
		const string_t returns
	) {
		check_name(name);
		check_new_name(name);
		check_type(returns);

		bool is_const = constant.has_value() && !constant.value().as<string_t>().empty();

		auto native = std::make_shared<NativeNode>(is_const, name, returns, file, linecount);


		if (takes_.has_value() && takes_.value().get_type() == sol::type::table) {
			
			sol::lua_table takes = takes_.value().as<sol::lua_table>();


;			for (size_t i = 0; i < takes.size(); i += 2) {
				const std::string& type = takes[i + 1];
				const std::string& name = takes[i + 2];

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



	parser["FunctionStart"] = [&](const string_t& constant, const string_t& name, std::optional<sol::lua_table> takes_, const string_t& returns) {
		check_name(name);
		check_new_name(name);
		check_type(returns);

		bool is_const = !constant.empty();

		auto func = std::make_shared<FunctionNode>(is_const, name, returns, file, linecount);

		if (takes_.has_value()) {
			auto& takes = takes_.value();
			for (size_t i = 0; i < takes.size(); i += 2) {
				const std::string& type = takes[i + 1];
				const std::string& name = takes[i + 2];

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
		}
		
		
		has_function = true;
		functions.save(name, func);

		block_stack.emplace_back(func);

		return (NodePtr)func;
	};

	parser["FunctionBody"] = [&]() {
		auto func = functions.back();

		bool has_return = false;

		for (auto& act : func->actions) {
			auto action = CastNode<ActionNode>(act);
			if (action->has_return) {
				has_return = true;
				break;
			}
		}

		if (!has_return && func->returns != "nothing") {
			if (func->has_return_any) {
				log.error(position(), "ERROR_RETURN_IN_ALL", func->name, func->returns);
			} else {
				log.error(position(), "ERROR_MISS_RETURN", func->name, func->returns);
			}
		}

	};

	parser["FunctionEnd"] = [&](const string_t& endfunction) {
		auto func = functions.back();

		if (endfunction.empty()) {
			log.error(position(), "ERROR_ENDFUNCTION", func->line);
		}

		block_stack.pop_back();

		return (NodePtr)func;
	};


	parser["Jass"] = [&](sol::object jass) {
		return (NodePtr)result.jass;
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

	
	//返回的节点必须是 jass 否则代表语法检测失败
	std::optional<NodePtr> res = peg_parser(jass_peg_rule, *config.script, parser);

	if (!res.has_value()) {
		return false;
	}
	
	NodePtr jass = res.value();
	if (!jass || jass->type != NodeType::JASS) {
		return false;
	}

	return true;
}