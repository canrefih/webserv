#ifndef STRING_BUILDER_HPP
#define STRING_BUILDER_HPP

#include <cstddef>
#include "lib/StringView.hpp"

class StringBuilder
{
	char		*_buf;
	std::size_t _len;
	std::size_t _cap;

	// private because any StringBuilder instance is immutable
	StringBuilder(const StringBuilder& other);
	StringBuilder& operator=(const StringBuilder& other);

public:

	static const std::size_t min_capacity = 256;

	StringBuilder();
	StringBuilder(std::size_t capacity);
	~StringBuilder();

	StringBuilder& put(char ch);
	StringBuilder& write(const char *s, std::size_t count);

	template <std::size_t N>
	StringBuilder& operator<<(char (&value)[N])
	{
		return write(value, N - 1);
	}

	StringBuilder& operator<<(int value);
	StringBuilder& operator<<(unsigned long value);
	StringBuilder& operator<<(long long value);
	StringBuilder& operator<<(char value);
	StringBuilder& operator<<(char *value);
	StringBuilder& operator<<(const std::string& value);
	StringBuilder& operator<<(const StringView& value);


	char		*data();
	char		*c_str();
	std::size_t capacity();
	std::size_t length();
	std::size_t size();

	void clear();
	void reserve(std::size_t count);

	// does give memory ownership to the returned StringView instance
	StringView release();
};

#endif // STRING_BUILDER_HPP
