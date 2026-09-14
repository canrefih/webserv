#include "lib/StringBuilder.hpp"

#include <cstring>
#include <sys/types.h>

//	char		*_buf;
//	std::size_t _len;
//	std::size_t _cap;

StringBuilder::StringBuilder() : _buf(NULL), _len(0), _cap(0) {}
StringBuilder::~StringBuilder() { delete[] _buf; }
StringBuilder::StringBuilder(std::size_t capacity)
	: _buf(new char[capacity]), _len(0), _cap(capacity) {}
StringBuilder::StringBuilder(const StringBuilder& other) { (void)other; }
StringBuilder& StringBuilder::operator=(const StringBuilder& other) { (void)other; return *this; }

char		*StringBuilder::data() const { return _buf; }
char		*StringBuilder::c_str() const { return _buf; }
std::size_t StringBuilder::capacity() const { return _cap; }
std::size_t StringBuilder::length() const { return _len; }
std::size_t StringBuilder::size() const { return _len; }
std::string	StringBuilder::str() const { return std::string(_buf, _len); }

void StringBuilder::clear()
{
	_len = 0;
	if (_buf)
		_buf[0] = 0;
}

StringView StringBuilder::release()
{
	StringView res(_buf, _len); 
	_buf = NULL ;
	_len = _cap = 0;
	return res;
}


void StringBuilder::reserve(std::size_t count)
{
	if (count >= _cap)
		_resize(count);
}

void	StringBuilder::_resize(std::size_t target_len)
{
	char	*new_buf;
	size_t	new_cap = 0;

	target_len += 1; // null terminator
	if (_cap == 0)
		new_cap = STRING_BUILDER_DEFAULT_CAPACITY;
	else
		new_cap = _cap;
	while (new_cap < target_len)
		new_cap *= 2;

	new_buf = new char[new_cap];
	std::memcpy(new_buf, _buf, _len);
	new_buf[_len] = '\0';
	delete[] _buf;
	_cap = new_cap;
	_buf = new_buf;
}

StringBuilder& StringBuilder::write(const char *s, std::size_t count)
{
	const std::size_t target_len = _len + count;

	if (target_len >= _cap)
		_resize(target_len);
	std::memcpy(_buf + _len, s, count);
	_len += count;
	_buf[_len] = '\0';
	return *this;
}

StringBuilder& StringBuilder::put(char ch)
{
	const std::size_t target_len = _len + 1;

	if (target_len >= _cap)
		_resize(target_len);
	_buf[_len] = ch;
	_buf[target_len] = '\0';
	_len = target_len;
	return *this;
}

StringBuilder& StringBuilder::operator<<(char value) { return this->put(value); }

StringBuilder& StringBuilder::operator<<(char *value)
{
	return this->write(value, std::strlen(value));
}

StringBuilder& StringBuilder::operator<<(const std::string& value)
{
	return this->write(value.c_str(), value.length());
}

StringBuilder& StringBuilder::operator<<(const StringView& value)
{
	return this->write(value.c_str(), value.length());
}

static inline std::size_t _get_int_len(long long n)
{
	std::size_t	res = 0;

	if (n < 0)
		res++;
	while (n != 0)
	{
		n /= 10;
		res++;
	}
	return res;
}

static inline std::size_t _get_uint_len(unsigned long n)
{
	std::size_t	res;

	res = 0;
	while (n != 0)
	{
		n /= 10;
		res++;
	}
	return (res);
}

static inline void _itoa(char *dest, int n, size_t char_count)
{
	bool	is_negative;
	ssize_t	i;

	is_negative = (n < 0);
	if (is_negative)
		n *= -1;
	i = char_count - 1;
	while (i >= 0 && !(is_negative && i == 0))
	{
		dest[i] = (n % 10) + '0';
		n /= 10;
		i--;
	}
	if (is_negative)
		dest[0] = '-';
	dest[char_count] = '\0';
}

static inline void _utoa(char *dest, unsigned long n, size_t char_count)
{
	ssize_t	i;

	i = char_count - 1;
	while (i >= 0)
	{
		dest[i] = (n % 10) + '0';
		n /= 10;
		i--;
	}
	dest[char_count] = '\0';
}

StringBuilder& StringBuilder::operator<<(long long value)
{
	const std::size_t number_len = _get_int_len(value);
	this->reserve(_len + number_len);
	_itoa(_buf + _len, value, number_len);
	_len += number_len;
	return *this;
}

StringBuilder& StringBuilder::operator<<(unsigned long value)
{
	const std::size_t number_len = _get_uint_len(value);
	this->reserve(_len + number_len);
	_utoa(_buf + _len, value, number_len);
	_len += number_len;
	return *this;
}

StringBuilder& StringBuilder::operator<<(unsigned int value)
{
	const std::size_t number_len = _get_uint_len(static_cast<unsigned long>(value));
	this->reserve(_len + number_len);
	_utoa(_buf + _len, static_cast<unsigned long>(value), number_len);
	_len += number_len;
	return *this;
}

StringBuilder& StringBuilder::operator<<(int value)
{
	const std::size_t number_len = _get_int_len(static_cast<int>(value));
	this->reserve(_len + number_len);
	_itoa(_buf + _len, static_cast<int>(value), number_len);
	_len += number_len;
	return *this;
}

StringBuilder& StringBuilder::operator<<(std::ostream& (*manip)(std::ostream&))
{
    if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::endl))
        return put('\n');

    if (manip == static_cast<std::ostream& (*)(std::ostream&)>(std::flush))
        return *this;

    return *this;
}

std::ostream& operator<<(std::ostream& out, const StringBuilder& sb)
{
	out.write(sb.data(), static_cast<std::streamsize>(sb.size()));
	return out;
}
