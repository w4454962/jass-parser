#include <iostream>

#include <tao/pegtl.hpp>

#include <boost/preprocessor.hpp>
#include <tao/pegtl/parse_error.hpp>

#undef or 
#undef and
#undef not
#define MACRO_DEF(r, def, s) def(s)
#define MACRO_CONCAT(r, head, s)  (BOOST_PP_CAT(head, s))

#pragma warning(disable:4309)

namespace tao::pegtl::jass {

	//ע��: �����//��ͷ �򲻶�ƥ��ǻ��в���
	struct comment : if_must<istring<'/', '/'>, plus< not_at<seq<one<'\r', '\n'>>>>> {};

	// :�ո�|tbl|utf-8�ļ�ͷ
	struct whilespace: sor<one<' ', '\t'>, istring<0xef, 0xbb, 0xbf>>{};

	// ƥ��0 ���� :ע��|�ո�
	struct space : star<sor<comment, whilespace>> {};

	// :�س�|����|�س�����
	struct line_char: sor<one<'\r', '\n'>, istring<'\r', '\n'>>{};

	// ƥ��һ��
	struct newline: seq<space, line_char> {};

	// ���Ե�ǰ��
	struct igone: star<not_at<line_char>>{};


	template< typename Key >
	struct key : seq<space, Key, not_at<identifier_other > > {};

#define KEYWORD(s)  \
	struct str_##s:TAO_PEGTL_STRING(#s) {}; \
	struct key_##s : key< str_##s > {};

//���ɹؼ���
#define KEYWORD_ALL (or)(and)(not)(if)(then)(endif) (loop)(endloop)(function)(endfunction) \
	(globals)(endglobals)(native)(takes)(set)(call)(returns)(return)(type)(extends)(constant) \
	(array)(local)(nothing)(integer)(real)(string)(code)(handle)(boolean)(debug)(true)(false)(null)

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

	struct name : seq<space, if_must<alpha, plus<sor<alnum, one<'_'>>>>> {};

	
	struct gt : istring<'>' >{};
	struct ge : istring<'>', '= '>{};
	struct lt : istring<'<' > {};
	struct le : istring<'<', '= '> {};
	struct eq : istring<'=', '= '> {};
	struct ue : istring<'!', '= '> {};

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
	struct exp: seq<exp_unit>{};


	struct grammar : seq<exp, eof> {};
}

int main(int argn, char** argv) {

	std::string ss = R"( name [ test[ 10 ] ])";

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