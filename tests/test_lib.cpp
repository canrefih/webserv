#include "lib/StringView.hpp"
#include "lib/StringBuilder.hpp"
#include "lib/HashMap.hpp"

#include <iostream>
#include <string>
#include <stdexcept>

#define YLW "\033[33m"
#define RST "\033[0m"

int main()
{
	std::cout << std::endl << YLW "> StringView tests" RST << std::endl << std::endl;

	{
		StringView test = "hello";
		std::cout << test << std::endl;
		for (StringView::iterator it = test.begin(); it != test.end(); ++it)
			std::cout << *it;
		std::cout << std::endl;
		for (std::size_t i = 0; i < test.length(); ++i)
			std::cout << test[i];
		std::cout << std::endl;
	}

	/**/
	/**/
	std::cout << std::endl << YLW "> StringBuilder tests" RST << std::endl << std::endl;
	/**/
	/**/

	{
		std::string str("bonjour~ruojnob");
		StringView test(str);

		while (test.length() != 0)
		{
			std::cout << test << std::endl;
			test.remove_prefix(1);
			test.remove_suffix(1);
		}

		std::cout << str << std::endl;
		StringView tidle = StringView(str).substr(str.find('~'), 1);
		std::cout << tidle << std::endl;
	}

	StringView receiver;
	{
		StringBuilder test;
		test << "- Bonjour!" << std::endl
			<< std::string("- Est ce que ca va?") << std::endl
			<< StringView("- Plutot bien.") << std::endl
			<< "- mon nombre magique est " << 42 <<  "." << std::endl;
		std::cout << test.str() << std::endl;
		receiver = test.release();
		std::cout << static_cast<void *>(test.data()) << std::endl;
	}
	std::cout << "received data:" << std::endl;
	std::cout << receiver << std::endl;
	// now the receiver have memory ownership
	receiver.destroy();

	/**/
	/**/
	std::cout << std::endl << YLW "> HashMap tests" RST << std::endl << std::endl;
	/**/
	/**/

	{
		HashMap<std::string, int> h;
		h["bonjour"] = 42;

		std::cout << h["bonjour"] << std::endl;
		h["bonjour"] = 21;
		std::cout << h["bonjour"] << std::endl;
	}
	std::cout << std::endl;
	{
		HashMap<int, StringView> h;
		h[404] = "Not Found";
		h[200] = "OK";
		h[400] = "Bad Request";

		for (HashMap<int, StringView>::iterator it = h.begin();
				it != h.end(); ++it)
		{
			std::cout << it->first << " " << it->second << std::endl;
		}
	}
	std::cout << std::endl;
	{
		// only h._data is heap memory, strings data are not 
		HashMap<StringView, StringView> h;
		h["bonjour"] = "hello";
		h["au revoir"] = "bye";
		h["tomate"] = "tomato";
		h["pomme"] = "apple";

		for (HashMap<StringView, StringView>::iterator it = h.begin();
				it != h.end(); ++it)
		{
			std::cout << it->first << ": " << it->second << std::endl;
		}

		std::cout << std::endl;
		const HashMap<StringView, StringView> c = h;

		for (HashMap<StringView, StringView>::const_iterator it = c.begin();
				it != c.end(); ++it)
		{
			std::cout << it->first << ": " << it->second << std::endl;
		}

		h.erase("tomate");
		h.erase("pomme");

		// prints nothing, default StringView constr is inserted in h["tomate"];
		std::cout << h["tomate"] << std::endl;

		try {
			h.at("non existant key");
		} catch (std::exception &e) {
			std::cout << e.what() << std::endl;
		}
	}
}
