#include "lib/StringView.hpp"

#include <cstring>

StringView::StringView() : _buf(NULL), _len(0) {}
StringView::~StringView() {}
StringView::StringView(const char* str) : _buf(str), _len(std::strlen(str)) {}
StringView::StringView(const char* str, StringView::size_type len) : _buf(str), _len(len)  {}
StringView::StringView(const std::string& str) : _buf(str.c_str()), _len(str.length()) {}
StringView::StringView(const StringView& other) : _buf(other._buf), _len(other._len) {}
StringView& StringView::operator=(const StringView& other)
{
	this->_buf = other._buf;
	this->_len = other._len;
	return *this;
}

StringView::const_reference StringView::operator[](StringView::size_type pos) const
{
	return _buf[pos];
}

StringView::const_reference StringView::at(StringView::size_type pos) const
{
	if (pos >= _len)
		throw std::out_of_range("StringView::at");
	return _buf[pos];
}

StringView::const_pointer StringView::data() const { return _buf; }
StringView::size_type StringView::size() const { return _len; }
StringView::size_type StringView::length() const { return _len; }
StringView::const_reference StringView::back() const { return _buf[_len - 1]; }
StringView::const_reference StringView::front() const { return _buf[0]; }
bool StringView::empty() const { return _len == 0; }

StringView::const_iterator StringView::begin() const { return _buf; }
StringView::const_iterator StringView::end() const { return (_len ? _buf + _len : _buf); }

void StringView::remove_prefix(StringView::size_type n)
{
	if (n > _len)
		n = _len;
	_buf += n;
	_len -= n;
}

void StringView::remove_suffix(StringView::size_type n)
{
	if (n > _len)
		n = _len;
	_len -= n;
}

StringView StringView::substr(StringView::size_type pos, StringView::size_type count) const
{
	if (pos > _len)
		throw std::out_of_range("StringView::substr");
	if (count > _len - pos)
		count = _len - pos;
	return StringView(_buf + pos, count);
}

// https://en.cppreference.com/cpp/string/basic_string_view/find
// All *find* member fonctions share the same overloads.
// Here is a macro that define automatically all corresponding overloads,
// actually just to reduce the amount of lines of code in this file.
#define STRINGVIEW_OVERLOADS(fn)\
StringView::size_type StringView::fn(char c, StringView::size_type pos) const\
{\
	return fn(StringView(&c, 1), pos);\
}\
\
StringView::size_type StringView::fn(const char* s, StringView::size_type pos, StringView::size_type count) const\
{\
	return fn(StringView(s, count), pos);\
}\
\
StringView::size_type StringView::fn(const char* s, StringView::size_type pos) const\
{\
	return fn(StringView(s), pos);\
}\

StringView::size_type StringView::find(const StringView& v, StringView::size_type pos) const
{
	if (pos >= _len)
		return StringView::npos;
	for (std::size_t i = pos; i < _len && (_len - i) >= v._len; ++i)
	{
		std::size_t j = 0; 
		for (; j < v._len; ++j)
		{
			if (_buf[i + j] != v._buf[j])
				break;
		}
		if (j == v._len)
			return i;
	}
	return StringView::npos;
}

STRINGVIEW_OVERLOADS(find)

StringView::size_type StringView::rfind(const StringView& v, StringView::size_type pos) const
{
	if (v._len > _len)
		return StringView::npos;

	if (pos >= _len)
		pos = (_len != 0 ? _len - 1 : 0);

	if (v._len > _len - pos)
		pos = _len - v._len;

	for (difference_type i = static_cast<difference_type>(pos); i >= 0; --i)
	{
		std::size_t j = 0;
		for (; j < v._len; ++j)
		{
			if (_buf[i + j] != v._buf[j])
				break;
		}
		if (j == v._len)
			return i;
	}
	return StringView::npos;
}

STRINGVIEW_OVERLOADS(rfind)

StringView::size_type StringView::find_first_of(const StringView& v, StringView::size_type pos) const
{
	if (pos >= _len)
		return StringView::npos;
	for (std::size_t i = pos; i < _len; ++i)
	{
		for (std::size_t j = 0; j < v._len; ++j)
		{
			if (_buf[i] == v._buf[j])
				return i;
		}
	}
	return StringView::npos;
}

STRINGVIEW_OVERLOADS(find_first_of)

StringView::size_type StringView::find_last_of(const StringView& v, StringView::size_type pos) const
{
	if (pos >= _len)
		pos = (_len != 0 ? _len - 1 : 0);
	for (difference_type i = static_cast<difference_type>(pos); i >= 0; --i)
	{
		for (std::size_t j = 0; j < v._len; ++j)
		{
			if (_buf[i] == v._buf[j])
				return i;
		}
	}
	return StringView::npos;
}

STRINGVIEW_OVERLOADS(find_last_of)

StringView::size_type StringView::find_first_not_of(const StringView& v, StringView::size_type pos) const
{
	if (pos >= _len)
		return StringView::npos;
	for (std::size_t i = pos; i < _len; ++i)
	{
		std::size_t j = 0;
		for (; j < v._len && _buf[i] != v._buf[j]; ++j)
			;
		if (j == v._len)
			return i;
	}
	return StringView::npos;
}

STRINGVIEW_OVERLOADS(find_first_not_of)

StringView::size_type StringView::find_last_not_of(const StringView& v, StringView::size_type pos) const
{
	if (pos >= _len)
		pos = (_len != 0 ? _len - 1 : 0);
	for (difference_type i = static_cast<difference_type>(pos); i >= 0; --i)
	{
		std::size_t j = 0;
		for (; j < v._len && _buf[i] != v._buf[j]; ++j)
			;
		if (j == v._len)
			return i;
	}
	return StringView::npos;
}

STRINGVIEW_OVERLOADS(find_last_not_of)

int StringView::compare(const StringView& v) const
{
    std::size_t i = 0;
    while (i < _len && i < v._len && _buf[i] == v._buf[i])
        i++;
    if (i == _len && i == v._len)
        return 0;
    if (i == _len)
        return -1;
    if (i == v._len)
        return 1;
    return static_cast<unsigned char>(_buf[i]) - static_cast<unsigned char>(v._buf[i]);
}

#undef STRINGVIEW_OVERLOADS

bool operator==(const StringView& lhs, const StringView& rhs)
{
	if (lhs.length() != rhs.length())
		return false;
	if (lhs.empty())
    	return true;
	return std::memcmp(lhs.data(), rhs.data(), lhs.length()) == 0;
}

bool operator!=(const StringView& lhs, const StringView& rhs)
{
	return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const StringView& sv)
{
	os.write(sv.data(), static_cast<std::streamsize>(sv.size()));
	return os;
}
