#ifndef STRING_VIEW_HPP
#define STRING_VIEW_HPP
#include <cstddef>
#include <string>
#include <ostream>

class StringView
{
	const char	*_buf;
	std::size_t	_len;

public:
	typedef char value_type;
	typedef const char* const_pointer;
	typedef const char& const_reference;
	typedef const char* iterator;
	typedef const char* const_iterator;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;

	static const size_type npos = static_cast<size_type>(-1);

	StringView();

	template <std::size_t N>
	StringView(const char (&str)[N]) : _buf(str), _len(N - 1) {};

	StringView(const char* str);
	StringView(const char* str, size_type len);
	StringView(const std::string& str);
	StringView(const StringView& other);

	StringView& operator=(const StringView& other);
	~StringView();

	const_reference operator[](size_type pos) const;
	const_reference at(size_type pos) const;
	const_pointer data() const;
	const_reference front() const;
	const_reference back() const;
	size_type size() const;
	size_type length() const;
	bool empty() const;

	const_iterator begin() const;
	const_iterator end() const;

	void remove_prefix(size_type n);
	void remove_suffix(size_type n);

	StringView substr(size_type pos = 0, size_type count = npos) const;

	size_type find(const StringView& v, size_type pos = 0) const;
	size_type find(char c, size_type pos = 0) const;
	size_type find(const char* s, size_type pos, size_type count) const;
	size_type find(const char* s, size_type pos = 0) const;

	size_type rfind(const StringView& v, size_type pos = npos) const;
	size_type rfind(char c, size_type pos = npos) const;
	size_type rfind(const char* s, size_type pos, size_type count) const;
	size_type rfind(const char* s, size_type pos = npos) const;
	
	size_type find_first_of(const StringView& v, size_type pos = 0) const;
	size_type find_first_of(char c, size_type pos = 0) const;
	size_type find_first_of(const char* s, size_type pos, size_type count) const;
	size_type find_first_of(const char* s, size_type pos = 0) const;
	
	size_type find_last_of(const StringView& v, size_type pos = npos) const;
	size_type find_last_of(char c, size_type pos = npos) const;
	size_type find_last_of(const char* s, size_type pos, size_type count) const;
	size_type find_last_of(const char* s, size_type pos = npos) const;
	
	size_type find_first_not_of(const StringView& v, size_type pos = 0) const;
	size_type find_first_not_of(char c, size_type pos = 0) const;
	size_type find_first_not_of(const char* s, size_type pos, size_type count) const;
	size_type find_first_not_of(const char* s, size_type pos = 0) const;
	
	size_type find_last_not_of(const StringView& v, size_type pos = npos) const;
	size_type find_last_not_of(char c, size_type pos = npos) const;
	size_type find_last_not_of(const char* s, size_type pos, size_type count) const;
	size_type find_last_not_of(const char* s, size_type pos = npos) const;

	int compare(const StringView& v) const;
};

bool operator==(const StringView& lhs, const StringView& rhs);
bool operator!=(const StringView& lhs, const StringView& rhs);

std::ostream& operator<<(std::ostream& os, const StringView& sv);

#endif // STRING_VIEW_HPP
