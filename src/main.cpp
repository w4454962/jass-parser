#include <iostream>

#include <tao/pegtl.hpp>

#include <boost/preprocessor.hpp>
#include <tao/pegtl/parse_error.hpp>

#undef or 
#undef and
#undef not
#define MACRO_DEF(r, def, s) def(s)
#define MACRO_CONCAT(r, head, s)  (BOOST_PP_CAT(head, s))


namespace tao::pegtl::jass {

	//注释: 如果是//开头 则不断匹配非换行部分
	struct comment : if_must<istring<'/', '/'>, plus< not_at<seq<one<'\r', '\n'>>>>> {};


	

	//struct sp: seq<sor<comment, >>

	template< typename Key >
	struct key : seq<Key, not_at<identifier_other > > {};

#define KEYWORD(s)  \
	struct str_##s:TAO_PEGTL_STRING(#s) {}; \
	struct key_##s : key< str_##s > {};

//生成关键字
#define KEYWORD_ALL (or)(and)(not)(if)(then)(endif) (loop)(endloop)(function)(endfunction) \
	(globals)(endglobals)(native)(takes)(set)(call)(returns)(return)(type)(extends)(constant) \
	(array)(local)(nothing)(integer)(real)(string)(code)(handle)(boolean)(debug)(true)(false)(null)

BOOST_PP_SEQ_FOR_EACH(MACRO_DEF, KEYWORD, KEYWORD_ALL)

	//关键字文本合集 
	struct sor_keyword : sor <BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(MACRO_CONCAT, str_, KEYWORD_ALL))> {};
	
	//关键字
	struct keyword : key<sor_keyword> {};

	//单斜杠转义符集合
	struct single: one<'a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '"', '\'', '0', '\n'>{};
	struct escaped: if_must<one<'\\'>, single>{};
	struct regular: not_one<'\r', '\n'>{};
	struct character: sor<escaped, regular>{};


	//字符串 匹配一对Q中间的内容
	template<char Q>
	struct short_string: if_must<one<Q>, until<one<Q>, character>>{};

	struct boolean: sor<key_true, key_false>{};

	struct string: sor<short_string<'"'>>{};

	struct digits: plus< digit> {};
	struct frac: seq<one<'.'>, digits > {};

	struct real: seq<opt<one< '-' >>, digits, opt<frac>> {};
	
	struct integer256: short_string<'\''>{};
	struct integer16: if_must<sor<istring<'0', 'x'>, one<'$'>>, plus<xdigit>>{};
	struct integer8: if_must<istring<'0'>, plus<odigit>> {};
	struct integer: seq<opt<one< '-' >>, sor<integer256, integer16, integer8, digits>> {};

	struct value: sor<key_null, boolean, string, real, integer>{};

	struct name : if_must<alpha, plus<sor<alnum, one<'_'>>>> {};

	struct exp;
	struct exp_unit;

	struct exp_neg: seq <opt<one< '-' >>, exp_unit>{};

	struct exp_var : seq<name, opt<seq<one<'['>, exp, one<']'>>>>{};
	struct exp_value: sor<value, exp_var>{};
	struct exp_call_args: seq<exp, star<seq<one<','>, exp>>>{};
	struct exp_call: seq<name, one<'('>, opt<exp_call_args>, one<'>'>>{};
	struct exp_code: seq<key_function, name, one<'('>, opt<exp_call_args>, one<')'>>{};
	struct exp_paren : seq<one<'('>, exp, one<')'>>{};
	struct exp_unit : sor<exp_paren, exp_code, exp_call, exp_value, exp_neg>{};
	struct exp: seq<exp_unit>{};




	struct grammar : seq<exp, eof> {};
}

int main(int argn, char** argv) {

	std::string ss = R"(10a)";

	tao::pegtl::memory_input in(ss.c_str(), "");

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