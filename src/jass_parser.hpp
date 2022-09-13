#pragma once
#include "stdafx.h"
#include "jass_peg_rule.h"
#include "mimalloc.h"
std::string_view convert_message(std::string_view msg);

int num = 0, num2 = 0;

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

inline bool string_equal(const string_t& first, const string_t& second) {
	return (first.data() == second.data() && first.size() == second.size()) || first == second;
}

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

#define IndexType lua_Integer
#define LoadIndex lua_tointeger
#define PushIndex lua_pushinteger

struct Cache {
	NodeContainer<string_t, struct ExpStringValue> string_nodes;
	NodeContainer<float, struct ExpRealValue> real_nodes;
	NodeContainer<int, struct ExpIntegerValue> integer_nodes;
	NodeContainer<string_t, struct ExpCodeValue> code_nodes;

	NodeContainer<void*, ExpNode> var_nodes;


	NodeContainer<string_t, struct Node> name2ecall;
	NodeContainer<string_t, struct Node> name2acall;


	NodeContainer<IndexType, struct Node> handle_map;
	

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

	IndexType node(NodePtr node);
	NodePtr node(IndexType handle);
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
	NATIVE,
	FUNCTION,
	JASS,
};

typedef std::function<bool(NodePtr node, size_t branch_index)> NodeFilter;


IndexType g_index = 0;

struct Node {
	NodeType type = NodeType::NONE;
	IndexType handle;

	Node() {
		handle = ++g_index;
	}

	virtual const NodeType get_type (){  return type; }
	virtual bool each_childs(const NodeFilter& filter) { return 0; }
	virtual size_t get_childs_size() { return 0; }
	virtual bool add_child(const NodePtr& node) { return 0; }
};


IndexType Cache::node(NodePtr node) {
	handle_map.save(node->handle, node);
	return node->handle;
}

NodePtr Cache::node(IndexType handle) {
	return handle_map.find(handle);
}


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

	gc_string_t file;
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
	std::unordered_set<string_t> childs;

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



struct NativeNode : Node {
	gc_string_t file;
	size_t line;
	bool is_const;
	gc_string_t name;
	gc_string_t returns;
	Container<ArgNode> args;

	NativeNode(bool is_const_, const string_t& name_, const string_t& file_, size_t line_)
		: is_const(is_const_),
		name(name_),
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

	FunctionNode(bool is_const_, const string_t& name_, const string_t& file_, size_t line_)
		: NativeNode(is_const_, name_, file_, line_)
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
			std::cout << msg->message << std::endl;
			msg->level = ErrorLevel::error;
			errors.emplace_back(msg);
			
		}
		catch (...) {
			std::cout << "Crash " << fmt << std::endl;
		}
	}

	template<typename... Args>
	void error_append(const std::string& fmt, Args... args)
	{
		if (errors.size() == 0)
			return;

		auto msg = errors.back();
		msg->message += std::vformat(convert_message(fmt), std::make_format_args(args...));
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




struct Jass : Node {
public:
	std::shared_ptr<Cache> gc;

	gc_string_t file;

	Container<TypeNode> types;
	Container<GlobalNode> globals;
	Container<NativeNode> natives;
	Container<FunctionNode> functions;

	std::unordered_map<string_t, std::shared_ptr<LocalNode>> exploits;

	std::unordered_map<size_t, string_t> comments;

	ParseLog log;


	std::unordered_set<string_t> keywords = {
		"globals", "endglobals", "constant", "native", "array", "and",
		"or", "not", "type", "extends", "function", "endfunction", "nothing",
		"takes", "returns", "call", "set", "return", "if", "then", "endif", "elseif",
		"else", "loop", "endloop", "exitwhen", "local", "true", "false"
	};

	Jass()
	{
		gc = std::make_shared<Cache>();
		functions.list.reserve(20000);
		globals.list.reserve(10000);
		natives.list.reserve(2000);

		type = NodeType::JASS;

		jass_gc = gc;

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

	void init(lua_State* L) {
		m_state = L;
		
		jass_gc = gc;

		null_value = std::make_shared<ExpNullValue>();
		jass_gc->node(null_value);

		true_value = std::make_shared<ExpBooleanValue>(true);
		jass_gc->node(true_value);

		false_value = std::make_shared<ExpBooleanValue>(false);
		jass_gc->node(false_value);
	}

	bool run(const ParseConfig& config) {
		linecount = 1;
		jass = this;
		has_function = false;
		finish = false;

		loop_stack_count = 0;

		jass_gc = gc;

		file = config.file;

		lua_State* L = m_state;

	
		lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");

		lua_getfield(L, -1, "peg");

		if (!lua_isfunction(L, -1)) {
			printf("not is function \n");
			lua_pop(L, 2);
			return false;
		}

		lua_pushlstring(L, jass_peg_rule, sizeof(jass_peg_rule) - 1);
		lua_pushlstring(L, config.script->data(), config.script->size());
		push_parser(L);
	
		if (LUA_OK != lua_pcall(L, 3, 0, 0)) {
			printf("error %s\n", lua_tostring(L, -1));
			lua_pop(L, 2);
			return false;
		}


		lua_pop(L, 1);

		return finish;
	}

private:

	auto position() {
		auto msg = std::make_shared<ParseErrorMessage>();
		msg->file = file;
		msg->line = linecount;
		msg->column = -1;
		//msg->at_line = relabel["line"](*config.script, linecount);

		return msg;
	};

	auto append_expolit_warning(VarPtr var) {
		//if (!config.exploit)
		//	return;

		if (!var || var->type == NodeType::VAR_ARG)
			return;
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

	auto cast_integer(const std::string& str, int flag) {
		int value;
		try {
			value = std::stoul(str, 0, flag);
		}
		catch (...) {
			log.error(position(), "WARNING_INTEGER_OVERFLOW", str);
		}

		auto node = jass_gc->integer_nodes.find(value);
		if (node) {
			return node->handle;
		}
		node = std::make_shared<ExpIntegerValue>(value);
		jass_gc->integer_nodes.save(value, node);

		return jass_gc->node(node);
	};

	auto check_name(const string_t& name) {
		if (keywords.find(name) != keywords.end()) {
			log.error(position(), "ERROR_KEY_WORD", name);
		}
	};

	auto check_new_name_types(const string_t& name) {
		auto type = types.find(name);
		if (type) {
			if (type->parent) {
				log.error(position(), "ERROR_REDEFINE_TYPE", name, type->file, type->line);
			}
			else {
				log.error(position(), "ERROR_DEFINE_NATIVE_TYPE", name);
			}
		}
	};
	auto check_new_name_globals(const string_t& name) {
		auto global = globals.find(name);
		if (global) {
			log.error(position(), "ERROR_REDEFINE_GLOBAL", name, global->file, global->line);
		}
	};

	auto check_new_name_functions(const string_t& name) {
		auto func = functions.find(name);
		if (func) {
			log.error(position(), "ERROR_REDEFINE_FUNCTION", name, func->file, func->line);
		}

		auto native = natives.find(name);
		if (native) {
			log.error(position(), "ERROR_REDEFINE_FUNCTION", name, native->file, native->line);
		}

	};

	auto check_new_name(const string_t& name) {
		check_new_name_types(name);
		check_new_name_globals(name);
		check_new_name_functions(name);
	};

	auto check_type(const string_t& type) {
		if (!type.empty() && !types.find(type)) {
			log.error(position(), "ERROR_UNDEFINE_TYPE", type);
		}
	};


	auto is_extends(const gc_string_t& type_name, const gc_string_t& parent_name) {
		auto type = types.find(type_name);
		auto parent = types.find(parent_name);

		if (!type || !parent) {
			return true;
		}

		if (string_equal(type->name, parent->name)) {
			return true;
		}
		return parent->childs.find(type_name) != parent->childs.end();
	};

	auto get_function (const string_t& name) {
		check_name(name);

		std::shared_ptr<NativeNode> func;

		func = functions.find(name);
		if (!func) {
			func = natives.find(name);
			if (!func) {
				log.error(position(), "FUNCTION_NO_EXISTS", name);
			}
		}
		return func;
	};

	auto check_call(const std::shared_ptr<NativeNode>& func, const std::shared_ptr<ExpCall>& call) {
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
				} else {
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


	auto get_variable(const string_t& name) {
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

			//if (config.exploit) {
			//	auto it = exploits.find(name);
			//	if (it != exploits.end()) {
			//		return (VarPtr)it->second;
			//	}
			//}

			return (VarPtr)global;
		}
		log.error(position(), "VAR_NO_EXISTS", name);

		return VarPtr();
	};

	auto check_set(VarPtr& var, bool need_array, ExpPtr index, ExpPtr exp) {
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

	auto check_get(VarPtr& var, bool need_array) {

		if (need_array) {
			if (var->type == NodeType::VAR_ARG || !CastNode<LocalNode>(var)->is_array) {
				log.error(position(), "ERROR_WASTE_INDEX", var->name);
				append_expolit_warning(var);
			}
		} else {
			if (var->type != NodeType::VAR_ARG && CastNode<LocalNode>(var)->is_array) {
				log.error(position(), "ERROR_NO_INDEX", var->name);
				append_expolit_warning(var);
			}
			if (!var->has_set) {
				//log.warning(position(), "ERROR_GET_UNINIT", var->name);
				//append_expolit_warning(var);
			}
		}
	};

	auto get_match_exp_type(hash_t ht1, hash_t ht2) {
		string_t exp_type;
		if (ht1 == "integer"s_hash) {
			if (ht2 == "integer"s_hash) {
				exp_type = "integer";
			} else if (ht2 == "real"s_hash) {
				exp_type = "real";
			}
		} else if (ht1 == "real"s_hash) {
			if (ht2 == "integer"s_hash || ht2 == "real"s_hash) {
				exp_type = "real";
			}
		}
		return exp_type;
	};

	auto get_connect_exp_type(hash_t ht1, hash_t ht2) {
		if ((ht1 == "string"s_hash || ht1 == "null"s_hash) && (ht2 == "string"s_hash || ht2 == "null"s_hash)) {
			return "string";
		}
		return "";
	};

	auto is_base_type_equal(const string_t& t1, const string_t& t2) {
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
		return string_equal(first->name, second->name);
	};

	auto get_binary_exp_type(const ExpPtr& first, const string_t& op, const ExpPtr& second) {
		uint32_t byte = 0;
		for (auto c : op) {
			byte = (byte << 8) | c;
		}

		string_t exp_type = "nothing";

		string_t t1 = first->vtype, t2 = second->vtype;

		auto ht1 = hash_s(t1), ht2 = hash_s(t2);

		switch (byte)
		{
		case '+':
			exp_type = get_match_exp_type(ht1, ht2); //如果是数字相加

			if (exp_type.empty()) { //否则 如果是 字符串连接
				exp_type = get_connect_exp_type(ht1, ht2);
			}
			if (exp_type.empty()) {
				log.error(position(), "ERROR_ADD", t1, t2);
			}
			break;
		case '-':
			exp_type = get_match_exp_type(ht1, ht2);
			if (exp_type.empty()) {
				log.error(position(), "ERROR_SUB", t1, t2);
			}
			break;

		case '*':
			exp_type = get_match_exp_type(ht1, ht2);
			if (exp_type.empty()) {
				log.error(position(), "ERROR_MUL", t1, t2);
			}
			break;

		case '/':
			exp_type = get_match_exp_type(ht1, ht2);
			if (exp_type.empty()) {
				log.error(position(), "ERROR_DIV", t1, t2);
			}
			break;

		case '%':
			if (ht1 == "integer"s_hash && ht2 == "integer"s_hash) {
				exp_type = t1;
			} else {
				log.error(position(), "ERROR_MOD");
			}
			break;

		case '==':
		case '!=':
			if (ht1 == "null"s_hash || ht2 == "null"s_hash) {
				exp_type = "boolean";
			} else if (!get_match_exp_type(ht1, ht2).empty()) {
				exp_type = "boolean";
			} else if (is_base_type_equal(t1, t2)) {
				exp_type = "boolean";
			} else {
				log.error(position(), "ERROR_EQUAL", t1, t2);
			}
			break;
		case '>':
		case '<':
		case '>=':
		case '<=':

			if (!get_match_exp_type(ht1, ht2).empty()) {
				exp_type = "boolean";
			} else {
				log.error(position(), "ERROR_COMPARE", t1, t2);
			}
			break;
		case 'and':
		case 'or':
			if (ht1 == "boolean"s_hash && ht2 == "boolean"s_hash) {
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


	auto check_local_with_args(const string_t& name, const string_t& type, bool is_array) {
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


	auto check_local_with_globals(const string_t& name, const string_t& type, bool is_array) {
		auto var = globals.find(name);
		if (!var) return;

		if (is_array && !var->is_array) {
			log.error(position(), "ERROR_REDEFINE_ARRAY_WITH_GLOBAL", name, var->file, var->line);
		} else {
			log.warning(position(), "ERROR_REDEFINE_GLOBAL", name, var->file, var->line);
		}
	};


	auto block_add_node(const NodePtr& node) {
		if (block_stack.empty()) {
			return;
		}

		auto& back = block_stack.back();
		//std::cout << "add child [" << back->type << "] << " << node->type << "\n";
		back->add_child(node);
	};



private:
	static int Nil(lua_State* L) {
		PushIndex(L, jass->null_value->handle);
		return 1;
	}

	static int nl(lua_State* L) {
		jass->linecount++;
		return 0;
	}

	static int File(lua_State* L) {
		lua_pushlstring(L, jass->file.data(), jass->file.size());
		return 1;
	}

	static int Line(lua_State* L) {
		PushIndex(L, jass->linecount);
		return 1;
	}

	static int Comment(lua_State* L) {
		size_t size = 0;
		std::string_view str(get_string(L, 1));
		jass->comments.emplace(jass->linecount, str);
		return 0;
	}

	static int l_NULL(lua_State* L) {
		PushIndex(L, jass->null_value->handle);
		return 1;
	}
	
	static int l_TRUE(lua_State* L) {
		PushIndex(L, jass->true_value->handle);
		return 1;
	}

	static int l_FALSE(lua_State* L) {
		PushIndex(L, jass->false_value->handle);
		return 1;
	}

	static int String(lua_State* L) {
		string_t str(get_string(L, 1));

		auto node = jass_gc->string_nodes.find(str);
		if (node) {
			PushIndex(L, node->handle);
			return 1;
		}
		node = std::make_shared<ExpStringValue>(str);
		jass_gc->string_nodes.save(node->value, node);

		PushIndex(L, jass_gc->node(node));
		return 1;
	};

	static int Real(lua_State* L) {
		float value = 0.f;
		try {
			size_t size = 0;
			std::string str = std::format("{}{}", get_string(L, 1), get_string(L, 2));

			value = std::stof(str);
		}
		catch (...) {

		}
		auto node = jass_gc->real_nodes.find(value);
		if (node) {
			PushIndex(L, node->handle);
			return 1;
		}
		node = std::make_shared<ExpRealValue>(value);
		jass_gc->real_nodes.save(value, node);

		PushIndex(L, jass_gc->node(node));
		return 1;
	}

	static int Integer8(lua_State* L) {

		std::string str = std::format("{}{}", get_string(L, 1), get_string(L, 2));

		PushIndex(L, jass->cast_integer(str, 8));
		return 1;
	}

	static int Integer10(lua_State* L) {

		std::string str = std::format("{}{}", get_string(L, 1), get_string(L, 2));

		PushIndex(L, jass->cast_integer(str, 10));
		return 1;
	}

	static int Integer16(lua_State* L) {

		std::string str = std::format("{}{}", get_string(L, 1), get_string(L, 2));

		PushIndex(L, jass->cast_integer(str, 16));

		return 1;
	}

	static int Integer256(lua_State* L) {

		string_t neg(get_string(L, 1));

		string_t str(get_string(L, 2));

		int i = 0;
		if (str.length() == 1) {
			i = str[0];
		} else if (str.length() == 4) {
			for (auto b : str) {
				if (b == '\\') {
					jass->log.error(jass->position(), "ERROR_INT256_ESC");
					break;
				}
				i = (i << 8) | b;
			}
		}
		i = neg.length() == 0 ? i : -i;
	
		auto node = jass_gc->integer_nodes.find(i);
		if (node) {
			PushIndex(L, node->handle);
			return 1;
		}
		node = std::make_shared<ExpIntegerValue>(i);
		jass_gc->integer_nodes.save(i, node);

		PushIndex(L, jass_gc->node(node));
		return 1;
	}

	static int Code(lua_State* L) {
		string_t name(get_string(L, 1));

		auto func = jass->get_function(name);
	
		
		if (lua_isstring(L, 2) && get_string(L, 2).size() > 0) {
			jass->log.error(jass->position(), "ERROR_CODE_HAS_CODE", name);
		}
		else if (func && func->args.list.size() > 0) {
			jass->log.warning(jass->position(), "ERROR_CODE_HAS_CODE", name);
		}
	
	
		auto node = jass_gc->code_nodes.find(name);
		if (node) {
			PushIndex(L, node->handle);
			return 1;
		}
		node = std::make_shared<ExpCodeValue>(name);
		jass_gc->code_nodes.save(node->value, node);
	
		PushIndex(L, jass_gc->node(node));

		return 1;
	}
	
	static int ECall(lua_State* L) {
		string_t name(get_string(L, 1));

		size_t args_count = lua_gettop(L);

		if (args_count == 1) { //如果没有参数 判断是否有缓存节点
			auto call = jass_gc->name2ecall.find(name);
			if (call) {
				PushIndex(L, call->handle);
				return 1;
			}
		}

		auto func = jass->get_function(name);

		if (!func) {
			PushIndex(L, 0);
			return 1;
		}

		auto call = std::make_shared<ExpCall>(func->name, func->returns);

		call->params.resize(args_count - 1);

		for (size_t i = 1; i < args_count; i++) {
			call->params[i - 1] = CastNode<ExpNode>(jass_gc->node(LoadIndex(L, i + 1)));
		}

		jass->check_call(func, call);
	
		if (args_count == 1) {//如果没有参数缓存节点
			jass_gc->name2ecall.save(name, call);
		}
		PushIndex(L, jass_gc->node(call));
		return 1;
	}

	static int Vari(lua_State* L) {
		string_t name(get_string(L, 1));

		IndexType index = LoadIndex(L, 2);

		auto var = jass->get_variable(name);
	
	
	
		if (!var) {
			PushIndex(L, 0);
			return 1;
		}
		jass->check_get(var, true);

		auto index_exp = CastNode<ExpNode>(jass_gc->node(index));
		auto exp_vari = std::make_shared<ExpVari>(var, index_exp);
	
		PushIndex(L, jass_gc->node(exp_vari));
		return 1;
	}

	static int Var(lua_State* L) {
		string_t name(get_string(L, 1));

		auto var = jass->get_variable(name);
		if (!var) {
			PushIndex(L, 0);
			return 1;
		}
		auto exp_var = jass_gc->var_nodes.find(var.get());
		if (!exp_var) {
			exp_var = std::make_shared<ExpVar>(var);
			jass_gc->var_nodes.save(var.get(), exp_var);
			jass_gc->node(exp_var);

			jass->check_get(var, false);
		} 
	
		PushIndex(L, exp_var->handle);
		return 1;
	}



	static int Neg(lua_State* L) {
		IndexType exp_node = LoadIndex(L, 1);

		auto exp = CastNode<ExpNode>(jass_gc->node(exp_node));

		if (exp->vtype != "real" && exp->vtype != "integer") {
			jass->log.error(jass->position(), "ERROR_NEG", exp->vtype);
		}

		auto exp_neg = std::make_shared<ExpNeg>(exp);

		PushIndex(L, jass_gc->node(exp_neg));

		return 1;
	};

	static int Binary(lua_State* L) {
		IndexType node = LoadIndex(L, 1);
		if (lua_gettop(L) > 1) {
				auto first = CastNode<ExpNode>(jass_gc->node(node));
				size_t size = lua_gettop(L);

				for (size_t i = 1; i < size; i += 2) {
					const string_t op(get_string(L, i + 1));
					auto second = CastNode<ExpNode>(jass_gc->node(LoadIndex(L, i + 2)));
					auto exp_type = jass->get_binary_exp_type(first, op, second);
					first = std::make_shared<ExpBinary>(exp_type, first, op, second);
				}
			
			node = jass_gc->node(first);
		}

		PushIndex(L, node);
		return 1;
	};


	static int Unary(lua_State* L) {
		IndexType node = LoadIndex(L, 1);

		size_t size = lua_gettop(L);
		if (size > 1) {
			auto first = CastNode<ExpNode>(jass_gc->node(LoadIndex(L, size)));

			for (size_t i = size - 1; i > 0; i--) {
				string_t op(lua_tostring(L, i));
				string_t exp_type = "boolean";
				if (op == "not" && first->vtype != "boolean") {
					jass->log.error(jass->position(), "ERROR_NOT_TYPE");
				}
				first = std::make_shared<ExpUnary>(exp_type, first, op);
			}		
			node = jass_gc->node(first);
		}
		PushIndex(L, node);
		return 1;
	}

	static string_t get_string(lua_State* L, size_t index) {
		//if (!lua_isstring(L, index)) {
		//	return string_t();
		//}
		size_t size = 0;
		const char* ptr = lua_tolstring(L, index, &size);
		return string_t(ptr, size);
	}


	static int Type(lua_State* L) {
		string_t name(get_string(L, 1));
		string_t extends(get_string(L, 2));


		jass->check_type(extends);
		jass->check_name(name);
		jass->check_new_name(name);

		auto parent = jass->types.find(extends);
		auto type = std::make_shared<TypeNode>(name, parent, jass->file, jass->linecount);

		jass->types.save(name, type);

		if (parent) { //继承置空能力
			if (auto it = parent->childs.find("null"); it != parent->childs.end()) {
				type->childs.emplace("null");
			}

			while (parent) {
				parent->childs.emplace(jass_gc->string(name));
				parent = parent->parent;
			}
		}

		return 0;
	};

	static int GlobalsStart(lua_State* L) {
		jass->globals_start_line = jass->linecount;
		return 0;
	}

	static int GlobalsEnd(lua_State* L) {
		string_t str(get_string(L, 1));

		if (str.size() == 0) {
			jass->log.error(jass->position(), "ERROR_ENDGLOBALS", jass->globals_start_line);
		}
		return 0;
	}

	static int Global(lua_State* L) {
		string_t constant(get_string(L, 1));
		string_t type(get_string(L, 2));
		string_t array(get_string(L, 3));
		string_t name(get_string(L, 4));

		NodePtr exp;
		if (lua_gettop(L) == 5) {
			exp = jass_gc->node(LoadIndex(L, 5));
		}
		jass->check_name(name);
		jass->check_new_name(name);
		jass->check_type(type);
	
		bool is_const = !constant.empty();
	
		if (is_const && !exp) {
			jass->log.error(jass->position(), "ERROR_CONSTANT_INIT");
		}

		bool is_array = !array.empty();
		if (is_array) {
			if (exp) {
				jass->log.error(jass->position(), "ERROR_ARRAY_INIT");
			}
			if (type == "code") {
				jass->log.error(jass->position(), "ERROR_CODE_ARRAY");
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
				jass->log.warning(jass->position(), "WARNING_NULL_NATIVE_IN_GLOBAL", call->name);
				break;
			case "GetObjectName"s_hash:
			case "CreateQuest"s_hash:
			case "CreateMultiboard"s_hash:
			case "CreateLeaderboard"s_hash:
				//这几个会崩溃
				jass->log.warning(jass->position(), "WARNING_CRASH_NATIVE_IN_GLOBAL", call->name);
				break;
			default:
				break;
			}
		}
		auto global = std::make_shared<GlobalNode>(is_const, type, name, is_array, jass->file, jass->linecount);
	
		global->exp = CastNode<ExpNode>(exp);
	
		if (global->exp) {
			VarPtr var = global;
			jass->check_set(var, is_array, ExpPtr(), global->exp);
		}
	
		jass->globals.save(name, global);

		return 0;
	}


	static int Local(lua_State* L) {
		string_t type(get_string(L, 1));
		string_t array(get_string(L, 2));
		string_t name(get_string(L, 3));

		bool is_array = !array.empty();

		jass->check_name(name);
		jass->check_new_name_functions(name);
		jass->check_new_name_types(name);
		jass->check_type(type);

		jass->check_local_with_args(name, type, is_array);
		jass->check_local_with_globals(name, type, is_array);

		if (is_array) {
			if (type == "code") {
				jass->log.error(jass->position(), "ERROR_CODE_ARRAY");
			}
		}
		auto local = std::make_shared<LocalNode>(type, name, is_array, jass->file, jass->linecount);

		//if (config.exploit) {
		//	auto global = globals.find(name);
		//	if (global && config.exploit) { //如果局部变量与全局变量同名 可以影响全局变量
		//		global->vtype = type;
		//		global->is_array = is_array;
		//
		//		exploits.emplace(gc_string_t(name), local);
		//	}
		//}

		//PushIndex(L, jass_gc->node(local));
		//auto local = CastNode<LocalNode>(jass_gc->node(LoadIndex(L, 1)));
		ExpPtr exp;
	
		if (lua_gettop(L) > 3) {
			exp = CastNode<ExpNode>(jass_gc->node(LoadIndex(L, 4)));

			local->has_set = true;
	
			local->exp = exp;
	
			if (local->is_array) {
				jass->log.error(jass->position(), "ERROR_ARRAY_INIT");
			}
	
			if (!jass->is_extends(exp->vtype, local->vtype)) {
				jass->log.error(jass->position(), "ERROR_SET_TYPE", local->name, local->vtype, exp->vtype);
			}
			auto func = jass->functions.back();
	
			if (func && exp->type == NodeType::EXP_CALL) { // 局部变量的初始值不能递归自己
				auto call = CastNode<ExpCall>(exp);
				if (func->name == call->name) {
					jass->log.error(jass->position(), "ERROR_LOCAL_RECURSION");
				}
			}
		}

		auto func = jass->functions.back();
	
		func->locals.save(local->name, local);
	
		return 0;
	}

	static int ACall(lua_State* L) {
		
		string_t name(get_string(L, 1));

		auto func = jass->get_function(name);
		auto current = jass->functions.back();
	
		if (current && current->is_const && func && !func->is_const) {
			jass->log.error(jass->position(), "ERROR_CALL_IN_CONSTANT", name);
		}
	
		size_t args_count = lua_gettop(L);

		if (args_count == 1) { //如果没有参数 判断是否有缓存动作节点
			auto action = jass_gc->name2acall.find(name);
			if (action) {
				jass->block_add_node(action);
				return 0;
			}
		}

		string_t exp_type = "nothing";
		if (func) {
			exp_type = func->returns;
		}

		auto call = std::make_shared<ExpCall>(name, exp_type);
		call->params.resize(args_count - 1);

		for (size_t i = 1; i < args_count; i++) {
			call->params[i - 1] = CastNode<ExpNode>(jass_gc->node(LoadIndex(L, i + 1)));
		}
		if (func) {
			jass->check_call(func, call);
		}
		
		auto action = std::make_shared<ActionCall>(call);
		
		jass->block_add_node(action);

		if (args_count == 1) {
			jass_gc->name2acall.save(name, action);
		}
		
		return 0;
	}

	static int Set(lua_State* L) {
		string_t name(get_string(L, 1));

		auto var = jass->get_variable(name);
		
		if (!var) {
			return 0;
		}

		if (lua_gettop(L) == 2) {
			auto exp = CastNode<ExpNode>(jass_gc->node(LoadIndex(L, 2)));
			jass->check_set(var, false, nullptr, exp);
			var->has_set = true;
		
			auto set = std::make_shared<ActionSet>(name, exp);
		
			jass->block_add_node(set);

		} else {
			auto index = CastNode<ExpNode>(jass_gc->node(LoadIndex(L, 2)));
			auto exp = CastNode<ExpNode>(jass_gc->node(LoadIndex(L, 3)));
			jass->check_set(var, true, index, exp);
		
			auto setindex = std::make_shared<ActionSetIndex>(name, index, exp);
		
			jass->block_add_node(setindex);
		}

		return 0;
	}

	static int Return(lua_State* L) {
		auto func = jass->functions.back();
		if (func && func->returns != "nothing") {
			jass->log.error(jass->position(), "ERROR_MISS_RETURN", func->name, func->returns);
		}
	
		auto ret = std::make_shared<ActionReturn>(nullptr);
	
		func->has_return_any = true;
	
		jass->block_add_node(ret);
		
		return 0;
	}

	static int ReturnExp(lua_State* L) {

		auto exp = CastNode<ExpNode>(jass_gc->node(LoadIndex(L, 1)));

		auto func = jass->functions.back();

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
					jass->log.warning(jass->position(), "ERROR_RETURN_INTEGER_AS_REAL", func->name, t1, t2);
				} else if (!jass->is_extends(t2, t1)) {
	
					//if (!config.rb) {
					//	log.error(position(), "ERROR_RETURN_TYPE", func->name, t2, t1);
					//	append_expolit_warning(var);
					//}
					//else {
					//
					//}
				}
			} else {
				jass->log.error(jass->position(), "ERROR_MISS_RETURN", func->name, t2);
			}
	
			if (func->is_const && exp->type == NodeType::EXP_VAR && !CastNode<ExpVar>(exp)->var->has_set) {
				jass->log.warning(jass->position(), "ERROR_CONSTANT_UNINIT", func->name);
			}
	
			func->has_return_any = true;
		} 

		auto ret = std::make_shared<ActionReturn>(exp);

		jass->block_add_node(ret);

		return 0;
	}

	static int Exit(lua_State* L) {
		if (jass->loop_stack_count == 0) {
			jass->log.error(jass->position(), "ERROR_EXITWHEN");
		}
		auto exp = CastNode<ExpNode>(jass_gc->node(LoadIndex(L, 1)));
		auto exit = std::make_shared<ActionExit>(exp);
		
		jass->block_add_node(exit);

		return 0;
	}

	static int IfStart(lua_State* L){

		auto action_if = std::make_shared<ActionIf>();

		jass->block_stack.emplace_back(action_if);

		auto if_node = std::make_shared<IfNode>(IfNode::TYPE::IF, jass->file, jass->linecount);

		jass->block_stack.emplace_back(if_node);

		return 0;
	};



	static int If(lua_State* L) {
		IndexType condition = LoadIndex(L, 1);
		auto exp = CastNode<ExpNode>(jass_gc->node(condition));
		if (exp->vtype != "boolean") {
			jass->log.warning(jass->position(), "ERROR_CONDITION_TYPE");
		}

		auto if_node = CastNode<IfNode>(jass->block_stack.back());

		if_node->condition = exp;

		for (auto& act : if_node->actions) {
			auto action = CastNode<ActionNode>(act);
			if (action->has_return) {
				if_node->has_return = true;
				break;
			}
		}

		jass->block_stack.pop_back();

		jass->block_add_node(if_node);

		return 0;
	};


	static int ElseifStart(lua_State* L){

		auto elseif_node = std::make_shared<IfNode>(IfNode::TYPE::ELSEIF, jass->file, jass->linecount);

		jass->block_stack.emplace_back(elseif_node);

		return 0;
	};

	static int Elseif(lua_State* L) {
		IndexType condition = LoadIndex(L, 1);

		auto exp = CastNode<ExpNode>(jass_gc->node(condition));
		if (exp->vtype != "boolean") {
			jass->log.error(jass->position(), "ERROR_CONDITION_TYPE");
		}

		auto elseif_node = CastNode<IfNode>(jass->block_stack.back());

		elseif_node->condition = exp;

		for (auto& act : elseif_node->actions) {
			auto action = CastNode<ActionNode>(act);
			if (action->has_return) {
				elseif_node->has_return = true;
				break;
			}
		}
		jass->block_stack.pop_back();

		jass->block_add_node(elseif_node);
		return 0;
	};

	static int ElseStart(lua_State* L) {

		auto else_node = std::make_shared<IfNode>(IfNode::TYPE::ELSE, jass->file, jass->linecount);

		jass->block_stack.emplace_back(else_node);

		return 0;
	};


	static int Else(lua_State* L) {
		auto else_node = CastNode<IfNode>(jass->block_stack.back());

		for (auto& act : else_node->actions) {
			auto action = CastNode<ActionNode>(act);
			if (action->has_return) {
				else_node->has_return = true;
				break;
			}
		}

		jass->block_stack.pop_back();
		jass->block_add_node(else_node);
		return 0;
	};

	static int Logic(lua_State* L) {
		auto action_if = CastNode<ActionIf>(jass->block_stack.back());

		if (!lua_isstring(L, 1) || strlen(lua_tostring(L, 1)) == 0) {
			auto if_node = action_if->if_nodes[0];
			jass->log.error(jass->position(), "ERROR_ENDIF", if_node->line);
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

		jass->block_stack.pop_back();

		jass->block_add_node(action_if);
		return 0;
	};

	static int LoopStart(lua_State* L) {
		auto loop = std::make_shared<ActionLoop>(jass->linecount);

		jass->block_stack.emplace_back(loop);

		jass->loop_stack_count++;
		PushIndex(L, jass->linecount);
		return 1;
	}


	static int Loop(lua_State* L) {
		size_t line = LoadIndex(L, 1);

		if (!lua_isstring(L, 2) || strlen(lua_tostring(L, 2)) == 0) {
			jass->log.error(jass->position(), "ERROR_ENDLOOP", line);
		}
		jass->loop_stack_count--;

		auto loop = CastNode<ActionLoop>(jass->block_stack.back());


		for (auto& act : loop->actions) {
			auto action = CastNode<ActionNode>(act);
			if (action->has_return) {
				loop->has_return = true;
				break;
			}
		}

		jass->block_stack.pop_back();

		jass->block_add_node(loop);
		return 0;
	};


	static int NativeName(lua_State* L) {
		size_t size = 0;

		bool is_const = lua_isstring(L, 1) && strlen(lua_tostring(L, 1)) > 0;

		string_t name(get_string(L, 2));

		jass->check_name(name);
		jass->check_new_name(name);

		auto native = std::make_shared<NativeNode>(is_const, name, jass->file, jass->linecount);

		jass->natives.save(name, native);

		return 0;
	};



	static int NArg(lua_State* L) {
		size_t size = 0;

		string_t type(get_string(L, 1));
		string_t name(get_string(L, 2));

		auto native = jass->natives.back();

		auto arg = std::make_shared<ArgNode>(type, name);

		auto var = jass->globals.find(name);
		if (var) {
			jass->log.error(jass->position(), "ERROR_REDEFINE_GLOBAL", name, var->file, var->line);
		}
		if (!jass->types.find(type)) {
			jass->log.error(jass->position(), "ERROR_UNDEFINE_TYPE", type);
		}
		native->args.save(name, arg);

		return 0;
	};

	static int NativeReturns(lua_State* L) {
		size_t size = 0;

		string_t returns(get_string(L, 1));

		jass->check_type(returns);

		auto native = jass->natives.back();

		native->returns = returns;

		return 0;
	};

	static int FunctionName(lua_State* L) {
		string_t constant(get_string(L, 1));
		string_t name(get_string(L, 2));

		jass->check_name(name);
		jass->check_new_name(name);


		bool is_const = !constant.empty();

		auto func = std::make_shared<FunctionNode>(is_const, name, jass->file, jass->linecount);

		jass->has_function = true;
		jass->functions.save(func->name, func);

		jass->block_stack.emplace_back(func);
		return 0;
	};


	static int FArg(lua_State* L) {
		size_t size = 0;

		string_t type(get_string(L, 1));
		string_t name(get_string(L, 2));

		auto func = jass->functions.back();

		auto arg = std::make_shared<ArgNode>(type, name);

		auto var = jass->globals.find(name);
		if (var) {
			jass->log.error(jass->position(), "ERROR_REDEFINE_GLOBAL", name, var->file, var->line);
		}
		if (!jass->types.find(type)) {
			jass->log.error(jass->position(), "ERROR_UNDEFINE_TYPE", type);
		}
		func->args.save(arg->name, arg);

		return 0;
	};

	static int FunctionReturns(lua_State* L) {
		size_t size = 0;

		string_t returns(get_string(L, 1));

		jass->check_type(returns);

		auto func = jass->functions.back();
		func->returns = returns;

		return 0;

	}

	static int FunctionBody(lua_State* L) {
		auto func = jass->functions.back();

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
				jass->log.error(jass->position(), "ERROR_RETURN_IN_ALL", func->name, func->returns);
			}
			else {
				jass->log.error(jass->position(), "ERROR_MISS_RETURN", func->name, func->returns);
			}
		}
		return 0;
	};

	static int FunctionEnd(lua_State* L) {

		auto func = jass->functions.back();

		if (!lua_isstring(L, 1)) {
			jass->log.error(jass->position(), "ERROR_ENDFUNCTION", func->line);
		}
		jass->block_stack.pop_back();
		return 0;
	}

	static int l_Jass(lua_State* L) {
		jass->finish = true;

		return 0;
	}

	static int returnAsReturns(lua_State* L) {
		jass->log.error(jass->position(), "ERROR_RETURN_AS_RETURNS");
		return 0;
	};

	static int setAsCall(lua_State* L) {
		jass->log.error(jass->position(), "ERROR_SET_AS_CALL");
		return 0;
	};

	static int callAsSet(lua_State* L) {
		jass->log.error(jass->position(), "ERROR_CALL_AS_SET");
		return 0;
	};


	static int constantLocal(lua_State* L) {
		jass->log.error(jass->position(), "ERROR_CONSTANT_LOCAL");
		return 0;
	};

	static int typeInFunction(lua_State* L) {
		jass->log.error(jass->position(), "ERROR_TYPE_IN_FUNCTION");
		return 0;
	};

	static int localInFunction(lua_State* L) {
		jass->log.error(jass->position(), "ERROR_LOCAL_IN_FUNCTION");
		return 0;
	};

	static int errorpos(lua_State* L) {

		int line = LoadIndex(L, 1);
		int col = LoadIndex(L, 2); 
		string_t at_line = get_string(L, 3);
		string_t err = get_string(L, 4);
	
		auto msg = std::make_shared<ParseErrorMessage>();
		msg->file = jass->file;
		msg->line = line;
		msg->column = col;
		msg->at_line = at_line;

		jass->log.error(msg, err);

		return 0;
	}
	

#define bind(n) lua_pushstring(L, #n);  \
	lua_pushcfunction(L, n);  \
	lua_rawset(L, -3);

#define bind2(s, n) lua_pushstring(L, s); \
	lua_pushcfunction(L, n);  \
	lua_rawset(L, -3);

	void push_parser(lua_State* L) {
		lua_newtable(L);

		bind(Nil);
		bind(nl);
		bind(File);
		bind(Line);
		bind(Comment);
		bind2("NULL", l_NULL);
		bind2("TRUE", l_TRUE);
		bind2("FALSE", l_FALSE);
		bind(String);
		bind(Real);
		bind(Integer8);
		bind(Integer10);
		bind(Integer16);
		bind(Integer256);
		bind(Code);
		bind(ECall);
		bind(Vari);
		bind(Var);
		bind(Neg);
		bind(Binary);
		bind(Unary);
		bind(Type);
		bind(GlobalsStart);
		bind(GlobalsEnd);
		bind(Global);
		bind(Local);
		bind(ACall);
		bind(Set);
		bind(Return);
		bind(ReturnExp);
		bind(Exit);
		bind(IfStart);
		bind(If);
		bind(ElseifStart);
		bind(Elseif);
		bind(ElseStart);
		bind(Else);
		bind(Logic);
		bind(LoopStart);
		bind(Loop);
		bind(NativeName);
		bind(NArg);
		bind(NativeReturns);
		bind(FunctionName);
		bind(FArg);
		bind(FunctionReturns);
		bind(FunctionBody);
		bind(FunctionReturns);
		bind(FunctionEnd);
		bind2("Jass", l_Jass);
		bind(returnAsReturns);
		bind(setAsCall);
		bind(callAsSet);
		bind(constantLocal);
		bind(typeInFunction);
		bind(localInFunction);
		bind(errorpos)

		//lua_pushvalue(L, -1);
	}

#undef bind
#undef bind2

	
private:
	lua_State* m_state;

	size_t linecount;

	static Jass* jass;

	NodePtr null_value, true_value, false_value;

	size_t globals_start_line, loop_stack_count;

	bool has_function, finish;

	Stack<NodePtr, 0x100> block_stack;
};

 Jass* Jass::jass;


