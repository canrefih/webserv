#include "utils.hpp"

#include <algorithm>

int utils::hex_to_val(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}


std::string utils::toLower(const std::string &str)
{
	std::string s = str;
	std::transform(s.begin(), s.end(), s.begin(), static_cast<int(*)(int)>(std::tolower));
	return s;
}
