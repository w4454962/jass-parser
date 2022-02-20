#include "stdafx.h"


hash_t hash_s(const std::string_view& str)
{
	hash_t ret{ basis };

	for (auto it = str.begin(); it != str.end(); it++) {

		ret ^= *it;
		ret *= prime;
	}
	return ret;
}
