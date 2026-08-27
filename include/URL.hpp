#ifndef URL_HPP
#define URL_HPP

#include <string>

class URL
{
public:
	static std::string encode(const std::string& raw);
	static std::string decode(const std::string& encoded);
};

#endif // URL_HPP
