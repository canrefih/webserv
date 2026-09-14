#ifndef STRING_BUILDER_HPP
#define STRING_BUILDER_HPP

#include "lib/StringView.hpp"
#include <cstddef>
#include <ostream>

#ifndef STRING_BUILDER_DEFAULT_CAPACITY
#define STRING_BUILDER_DEFAULT_CAPACITY 256
#endif

class StringBuilder
{
	char		*_buf;
	std::size_t _len;
	std::size_t _cap;

	// private because StringBuilder instances are immutable
	StringBuilder(const StringBuilder& other);
	StringBuilder& operator=(const StringBuilder& other);

	void _resize(std::size_t target_len);

public:

	StringBuilder();
	StringBuilder(std::size_t capacity);
	~StringBuilder();

	StringBuilder& put(char ch);
	StringBuilder& write(const char *s, std::size_t count);

	template <std::size_t N>
	StringBuilder& operator<<(const char (&value)[N])
	{
		return write(value, N - 1);
	}

	StringBuilder& operator<<(unsigned long value);
	StringBuilder& operator<<(long long value);
	StringBuilder& operator<<(unsigned int value);
	StringBuilder& operator<<(int value);
	StringBuilder& operator<<(char value);
	StringBuilder& operator<<(char *value);
	StringBuilder& operator<<(const std::string& value);
	StringBuilder& operator<<(const StringView& value);
	StringBuilder& operator<<(std::ostream& (*manip)(std::ostream&));


	char		*data() const;
	char		*c_str() const;
	std::size_t capacity() const;
	std::size_t length() const;
	std::size_t size() const;
	std::string	str() const;

	void clear();
	void reserve(std::size_t count);

	// does give memory ownership to the returned StringView instance
	StringView release();
};

std::ostream& operator<<(std::ostream& out, const StringBuilder& sb);

#endif // STRING_BUILDER_HPP
