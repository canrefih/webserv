#ifndef HASH_FN
#define HASH_FN

#include <cstddef>

template <typename T>
struct Hash
{
	std::size_t operator()(const T& val) const
	{
		return static_cast<std::size_t>(val);
	}
};

class HashFn
{
public:
	static std::size_t djb2(const char *s, const std::size_t len);
};

#endif // HASH_FN
