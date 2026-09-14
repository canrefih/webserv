#include "lib/StringView.hpp"
#include "lib/StringBuilder.hpp"
#include "lib/HashMap.hpp"

#include <iostream>
#include <iterator>
#include <string>
#include <cassert>
#include <stdexcept>

#define YLW "\033[33m"
#define RST "\033[0m"

// stateful hasher: every instance hashes differently (copy ctor regression test)
static std::size_t g_salt = 1000;
struct SaltedHash
{
	std::size_t salt;
	SaltedHash() : salt(g_salt++) {}
	std::size_t operator()(int k) const { return static_cast<std::size_t>(k) * 2654435761u ^ salt; }
};

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
			assert(false && "should throw std::out_of_range error");
		} catch (std::exception &e) {
			std::cout << e.what() << std::endl;
		}

		assert(std::distance(h.begin(), h.end()) == static_cast<std::ptrdiff_t>(h.size()));
	}

	/**/
	/**/
	std::cout << std::endl << YLW "> HashMap regression tests" RST << std::endl << std::endl;
	/**/
	/**/

	{
		// erase must remove the requested key, even on collision
		HashMap<int, int> h;
		h.insert(std::make_pair(0, 111)); // hash 0|2 = 2 -> slot 2
		h.insert(std::make_pair(2, 222)); // hash 2|2 = 2 -> collision
		assert(h.erase(2) == 1);
		assert(h.size() == 1);
		assert(h.at(0) == 111);
		try {
			h.at(2);
			assert(false && "key 2 should be gone");
		} catch (const std::out_of_range&) {}
		assert(h.erase(2) == 0); // already erased
		std::cout << "erase removes the right key on collision: OK" << std::endl;
	}
	{
		// duplicate insert reports failure and preserves the old value
		HashMap<int, int> h;
		h.insert(std::make_pair(7, 1));
		std::pair<HashMap<int, int>::iterator, bool> res = h.insert(std::make_pair(7, 2));
		assert(res.second == false);
		assert(h.at(7) == 1);
		assert(h.size() == 1);
		std::cout << "duplicate insert: OK" << std::endl;
	}
	{
		// probing a full table (no EMPTY slot) must not return a bogus slot
		HashMap<int, int> h(5);
		h.max_load_factor(1.1f);
		for (int i = 0; i < 5; ++i)
			h.insert(std::make_pair(i, i * 10));
		try {
			h.at(99);
			assert(false && "at() on absent key should throw");
		} catch (const std::out_of_range&) {}
		assert(h.find(99) == h.end());
		std::cout << "full table lookup: OK" << std::endl;
	}
	{
		// zero-capacity construction is clamped, no crash
		HashMap<int, int> h(0);
		h.insert(std::make_pair(1, 1));
		assert(h.size() == 1);
		assert(h.at(1) == 1);
		std::cout << "zero capacity: OK" << std::endl;
	}
	{
		// copy constructor must carry the hasher state, copies stay independent
		HashMap<int, int, SaltedHash> a;
		a.insert(std::make_pair(42, 4200));
		HashMap<int, int, SaltedHash> b(a);
		try {
			assert(b.at(42) == 4200);
		} catch (const std::out_of_range&) {
			assert(false && "copy must find the keys of the original");
		}
		assert(b.erase(42) == 1);
		assert(b.empty());
		assert(a.at(42) == 4200); // original untouched
		std::cout << "copy with stateful hasher: OK" << std::endl;
	}
	{
		// iterator -> const_iterator conversion, mixed comparison, erase(pos)
		HashMap<int, int> h;
		for (int i = 0; i < 10; ++i)
			h.insert(std::make_pair(i, i));
		HashMap<int, int>::const_iterator cit = h.begin();
		assert(cit != h.end());
		HashMap<int, int>::iterator next = h.erase(h.begin());
		(void)next;
		assert(h.size() == 9);
		while (h.begin() != h.end())
			h.erase(h.begin());
		assert(h.empty());
		std::cout << "iterator conversion + erase(pos): OK" << std::endl;
	}
	{
		// tombstone reuse: erase then re-insert keeps bookkeeping sane
		HashMap<int, int> h;
		h.insert(std::make_pair(1, 10));
		h.insert(std::make_pair(2, 20));
		assert(h.erase(1) == 1);
		h.insert(std::make_pair(3, 30));
		h.insert(std::make_pair(1, 100)); // re-insert the erased key
		assert(h.size() == 3);
		assert(h.at(1) == 100);
		assert(h.at(2) == 20);
		assert(h.at(3) == 30);
		assert(std::distance(h.begin(), h.end()) == static_cast<std::ptrdiff_t>(h.size()));
		std::cout << "tombstone reuse: OK" << std::endl;
	}
	{
		// rehash preserves every value (128 -> 256 -> ... -> 2048)
		HashMap<int, int> h;
		for (int i = 0; i < 1000; ++i)
			h.insert(std::make_pair(i, i * 3));
		assert(h.size() == 1000);
		for (int i = 0; i < 1000; ++i)
			assert(h.at(i) == i * 3);
		std::cout << "rehash integrity (1000 entries): OK" << std::endl;
	}
}
