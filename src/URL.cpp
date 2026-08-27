#include "URL.hpp"
#include <sstream>

std::string URL::encode(const std::string& raw)
{
	static const char hex_base[] = "0123456789ABCDEF";

	std::string res;
	res.reserve(raw.length() * 3);

	char	buf[4] = {0};
	unsigned char c;

	for (std::string::const_iterator it = raw.begin(); it != raw.end(); ++it)
	{
		c = *it;
		if (std::isalnum(c) || c == '-' || c == '_'
			|| c == '.' || c == '~') {
			buf[0] = c;
			buf[1] = '\0';
		} else {
			buf[0] = '%';
			buf[1] = hex_base[c >> 4];
			buf[2] = hex_base[c & 0x0F];
		}
		res += buf;
	}
	return res;
}

static int hex_to_val(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 10;
	return -1;
}

std::string URL::decode(const std::string& encoded)
{
	std::istringstream data(encoded);
	std::string res;
	res.reserve(encoded.length());

	int percent_code;
	int tmp;
	unsigned char c;
	unsigned char n;

	while (data >> c)
	{
		if (c == '%')
		{
			data >> c;
			tmp = hex_to_val(c);
			if (tmp == -1)
			{
				res += '%' + c;
				continue;
			}
			percent_code = tmp << 4;

			data >> n;
			tmp = hex_to_val(n);
			if (tmp == -1)
			{
				res += '%' + c + n;
				continue;
			}
			percent_code |= tmp;

			c = static_cast<char>(percent_code);
		}
		res += c;
	}
	return res;
}
