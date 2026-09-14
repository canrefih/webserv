#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <sstream>

namespace utils 
{

	// return converted int value or -1 in case of error
	int hex_to_val(char c);

	// equivalent to std::to_string(T) (from c++ 11)
	template <typename T>
	std::string to_string(T val)
	{
		std::ostringstream s;
		s << val;
		return s.str();
	}

	// Helper function to convert a string to lowercase for case-insensitive header name comparisons
	std::string toLower(const std::string &str);
}


#endif // UTILS_HPP
