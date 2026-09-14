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

}


#endif // UTILS_HPP
