#include <iostream>

#include <tao/pegtl.hpp>

#include <boost/preprocessor.hpp>
#include <boost/preprocessor/iteration/local.hpp>

#include <tao/pegtl/parse_error.hpp>

#undef or 
#undef and
#undef not
#define MACRO_DEF(r, def, s) def(s)
#define MACRO_CONCAT(r, head, s)  (BOOST_PP_CAT(head, s))

#pragma warning(disable:4309)

namespace tao::pegtl::jass {


	struct line_char : sor<one<'\r', '\n'>, istring<'\r', '\n'>> {};

	struct comment : if_must<two<'/'>, star<seq<not_at<line_char>, bytes<1>>>> {};

	struct whilespace: sor<one<' ', '\t'>, istring<0xef, 0xbb, 0xbf>>{};

	struct space : star<sor<comment, whilespace>> {};

	struct newline: seq<space, line_char> {};

	struct igone: star<not_at<line_char>>{};

	template< typename Key >
	struct key : seq<space, Key, not_at<identifier_other > > {};

#define KEYWORD(s)  \
	struct str_##s:TAO_PEGTL_STRING(#s) {}; \
	struct key_##s : key< str_##s > {};

//���ɹؼ���
#define KEYWORD_ALL	(or)(and)(not)(if)(then)(elseif)(else)(endif) (loop)(endloop)(function)(endfunction) \
	(globals)(endglobals)(native)(takes)(set)(call)(returns)(return)(exitwhen)(type)(extends)(constant) \
	(array)(local)(nothing)(debug)(true)(false)(null)

BOOST_PP_SEQ_FOR_EACH(MACRO_DEF, KEYWORD, KEYWORD_ALL)

 
	//�ؼ����ı��ϼ� 
	//struct sor_keyword : sor <BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(MACRO_CONCAT, str_, KEYWORD_ALL))> {};
	//�ؼ���
	//struct keyword : key<sor_keyword> {};

	
	struct comma: seq<space, one<','>>{};

	struct assign: seq<space, one<'='>>{};
	

	//��б��ת�������
	struct single: one<'a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '"', '\'', '0', '\n'>{};
	struct escaped: if_must<one<'\\'>, single>{};
	struct regular: not_one<'\r', '\n'>{};
	struct character: sor<escaped, regular>{};


	//�ַ��� ƥ��һ��Q�м������
	template<char Q>
	struct short_string: if_must<one<Q>, until<one<Q>, character>>{};

	struct boolean: sor<key_true, key_false>{};

	struct string: seq<space, short_string<'"'>>{};

	struct digits: plus< digit> {};
	struct frac: seq<one<'.'>, digits > {};

	struct real: seq<space, opt<one< '-' >>, space, digits, opt<frac>> {};
	
	struct integer256: seq<space, short_string<'\''>>{};
	struct integer16: if_must<sor<istring<'0', 'x'>, one<'$'>>, plus<xdigit>>{};
	struct integer8: if_must<istring<'0'>, plus<odigit>> {};

	struct integer: seq<space, opt<one< '-' >>, space, sor<integer256, integer16, integer8, digits>> {};

	struct value: sor<key_null, boolean, string, real, integer>{};

	struct name : seq<space, seq<alpha, opt<plus<sor<alnum, one<'_'>>>>>> {};

	
	struct gt : istring<'>' >{};
	struct ge : istring<'>', '='>{};
	struct lt : istring<'<' > {};
	struct le : istring<'<', '='> {};
	struct eq : istring<'=', '='> {};
	struct ue : istring<'!', '='> {};

	struct add : one<'+'> {};
	struct sub : one<'-'> {};
	struct mul : one<'*'> {};
	struct div : one<'/'> {};
	struct mod : one<'%'> {};

	
	struct pl : seq<space, one<'('>>{};
	struct pr : seq<space, one<')'>> {};
	struct bl : seq<space, one<'['>> {};
	struct br : seq<space, one<']'>> {};

	struct exp;
	struct exp_unit;

	struct exp_neg: seq<space, sub, exp_unit>{};
	struct exp_var : seq<name, opt<seq<bl, exp, br>>>{};
	struct exp_value: sor<value, exp_var>{};
	struct exp_call_args: seq<exp, star<seq<comma, exp>>>{};
	struct exp_call: seq<name, pl, opt<exp_call_args>, pr>{};
	struct exp_code: seq<key_function, name, pl, opt<exp_call_args>, pr>{};
	struct exp_paren : seq<pl, exp, pr>{};
	struct exp_unit : sor<exp_paren, exp_code, exp_call, exp_value, exp_neg>{};

	struct exp_add_sub: sor<add, sub>{};
	struct exp_mul_div: sor<mul, div>{};

	struct exp_not : key_not{};
	struct exp_compare_operator: sor< ue, eq, le, lt, ge, gt>{};
	struct exp_or : key_or{};
	struct exp_and: key_and{};

	struct exp_check_mul: seq<exp_unit, star<seq<space, exp_mul_div, exp_unit>>> {};
	struct exp_check_add: seq<exp_check_mul, star<seq<space, exp_add_sub, exp_check_mul>>>{};
	struct exp_check_not: seq<space, star<exp_not>, exp_check_add>{};
	struct exp_check_compare: seq<exp_check_not, star<seq<space, exp_compare_operator, exp_check_not>>> {};
	struct exp_check_or: seq<exp_check_compare, star<seq<space, exp_or, exp_check_compare>>>{};
	struct exp_check_and : seq<exp_check_or, star<seq<space, exp_and, exp_check_or>>> {};
	struct exp: exp_check_and {};

	struct type_extends: seq<key_type, name, key_extends, name>{};

	struct global : seq <space, not_at<key_globals, key_function, key_native>, opt<key_constant>, name, opt<key_array>, name, opt<seq<assign, exp>>> {};
	struct globals: seq<space, key_globals, newline, star<sor<global, newline>>, key_endglobals>{};

	struct local: seq<opt<key_constant>, key_local, name, opt<key_array>, name, opt<seq<assign, exp>>>{};
	struct local_list: star<sor<local, newline>> {};

	struct action;
	struct action_list : star<sor<action, newline>> {};

	struct action_call:seq<opt<key_debug>, key_call, exp_call>{};
	struct action_set:seq<key_set, exp_var, assign, exp>{};
	struct action_return: seq<key_return, sor<opt<exp>, space>>{};
	struct action_exitloop: seq<key_exitwhen, exp>{};

	struct if_statement : seq<key_if, exp, key_then, newline, action_list> {};
	struct elseif_statement : seq<key_elseif, exp, key_then, newline, action_list> {};
	struct else_statement : seq<key_else, newline, action_list> {};

	struct action_if: seq<if_statement, star<elseif_statement>, opt<else_statement>, key_endif>{};

	struct action_loop: seq<key_loop, action_list, key_endloop>{};

	struct action: sor<action_call, action_set, action_return, action_exitloop, action_if, action_loop>{};

	struct arg_define: seq<name, name>{};

	struct args_define : seq < sor < key_nothing, seq < arg_define, star<seq<comma, arg_define>> >> > {};

	struct native: seq<key_native, name, key_takes, args_define, key_returns, name>{};

	struct function: seq<key_function, name, key_takes, args_define, key_returns, name, newline, local_list, action_list, key_endfunction>{};

	struct chunk: sor<type_extends, globals, native, function, newline>{};

	struct jass: star<chunk>{};

	struct grammar : seq<jass, eof> {};
}


int main(int argn, char** argv) {

	std::string ss = R"(
   //test
globals
	integer test 
endglobals
    //
   //1

 native GetUnitName takes nothing returns string
 native GetUnitName takes unit handle returns string
 native GetUnitName takes unit handle, unit handle2 returns string

function GetUnitName takes unit handle returns string
	local integer i
	local integer b = 10
	local integer c = GetUnitName()
	local integer array d
	
	call GetUnitName()

	set i = 0

	set i = i + 1

	set d[1] = 0

	set d[i] = d[i] + 1 - 1

	loop
		exitwhen i == 10 

		if true then 
			set i = i + 1
		elseif 1+1 == 2 then
			set i = i + 1
		elseif false then 
			set i = i + 1
		else 
			loop 
				exitwhen i == 10 
				if true then 

				endif 
			endloop
		endif 
	endloop

endfunction

  
)";


	tao::pegtl::memory_input in(ss.c_str(), ss.c_str() + ss.size());

	try {
		auto ret = tao::pegtl::parse<tao::pegtl::jass::grammar>(in);

		std::cout << ret << std::endl;

	} catch(const tao::pegtl::parse_error& e) {
		const auto p = e.positions().front();
		std::cerr << e.what() << std::endl
			<< in.line_at(p) << std::endl
			<< std::setw(p.column) << '^' << std::endl;
	}
	return 0;
}