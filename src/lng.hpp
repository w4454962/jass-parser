#pragma once

#include "stdafx.h"



namespace tao::pegtl::lng {

	struct line_char : sor<one<'\r', '\n'>, istring<'\r', '\n'>> {};

	struct comment : if_must<two<'-'>, star<seq<not_at<line_char>, bytes<1>>>> {};

	struct whilespace : sor<one<' ', '\t'>, istring<0xef, 0xbb, 0xbf>> {};

	struct space : star<sor<comment, whilespace>> {};

	struct newline : seq<space, line_char> {};

	struct begin : one<'['> {};

	struct end: one<']'>{};

	struct key : plus<seq<not_one<'[', ']'>>> {};

	struct key_statement : if_must<begin, key, end>{};

	struct special: seq<begin, key, end, seq<not_at<newline>>> {};

	struct value : must<star<seq<sor<special, not_one<'['>>>>>  {};

	struct item : seq<space, seq<key_statement, newline>, value> {};

	struct lng : star<sor<item, newline>> {};

	struct grammar : seq<lng, opt<eof>> {};


	template< typename Rule >
	using selector = pegtl::parse_tree::selector<
		Rule,
		pegtl::parse_tree::store_content::on<item, key, value>
		>;

	template< typename Rule >
	using control = pegtl::normal< Rule >;

}