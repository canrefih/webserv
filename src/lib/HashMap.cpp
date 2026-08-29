
#include "lib/HashMap.hpp"

// http://www.cse.yorku.ca/~oz/hash.html
std::size_t djb2(const char *s, const size_t len)
{
	std::size_t	hash;

	hash = 5381;
	for (std::size_t i = 0; i < len; ++i)
		hash = ((hash << 5) + hash) + static_cast<unsigned char>(s[i]);

	return (hash);
}
