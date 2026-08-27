#include "URL.hpp"
#include <iostream>

struct TestData {
	std::string decoded;
	std::string encoded;
};

std::ostream& operator<<(std::ostream& out, TestData data)
{
	out << "{" << std::endl
		<< "decoded: " << data.decoded << std::endl
		<< "encoded: " << data.encoded << std::endl
		<< "}";
	return out;
}

enum TestType {
	TEST_ENCODE,
	TEST_DECODE,
};

bool do_test(TestData data, TestType test_type)
{
	std::string result;
	std::string expected;
	switch (test_type)
	{
		case TEST_ENCODE: {
			result = URL::encode(data.decoded);
			expected = data.encoded;
		} break;
		case TEST_DECODE: {
			result = URL::decode(data.encoded);
			expected = data.decoded;
		} break;
		default: break;
	}
	if (result != expected)
	{
		std::cout << std::endl << "Error!" << std::endl << std::endl
			<< "test data: " << data << std::endl
					<< "result:\t\t" << result << std::endl
					<< "expected:\t" << expected << std::endl << std::endl;
		return false;
	}
	return true;
}

int main(void)
{
	TestData tests[] = {
		{"hello world","hello%20world"},
		{"user@example.com","user%40example.com"},
		{"café au lait","caf%C3%A9%20au%20lait"},
		{"https://example.com/path?a=1&b=2","https%3A%2F%2Fexample.com%2Fpath%3Fa%3D1%26b%3D2"},
		{"100%","100%25"},
		{"a+b=c","a%2Bb%3Dc"},
		{"file name (final).txt","file%20name%20%28final%29.txt"},
		{"日本語","%E6%97%A5%E6%9C%AC%E8%AA%9E"},
		{"emoji 🚀","emoji%20%F0%9F%9A%80"},
		{"query=what's up?","query%3Dwhat%27s%20up%3F"},
		{"C:\\Users\\name\\docs","C%3A%5CUsers%5Cname%5Cdocs"},
		{"price: 10€","price%3A%2010%E2%82%AC"},
		{"a&b#c!d","a%26b%23c%21d"},
		{"array[0]=test","array%5B0%5D%3Dtest"},
		{"{\"key\": \"value\"}","%7B%22key%22%3A%20%22value%22%7D"},
		{"résumé_été-2024.pdf","r%C3%A9sum%C3%A9_%C3%A9t%C3%A9-2024.pdf"},
		{"x < y > z","x%20%3C%20y%20%3E%20z"},
		{"semi;colon:slash/split","semi%3Bcolon%3Aslash%2Fsplit"},
		{"München Übergröße","M%C3%BCnchen%20%C3%9Cbergr%C3%B6%C3%9Fe"},
		{"~tilde_pipe|","~tilde_pipe%7C"}
	};

	const std::size_t tests_count = sizeof(tests)/sizeof(TestData);
	std::cout << "Encoding tests\n"; 
	for (std::size_t i = 0; i < tests_count; ++i)
		if (!do_test(tests[i], TEST_ENCODE)) return 1;

	std::cout << "OK!\n";
	std::cout << "Decoding tests\n"; 
	for (std::size_t i = 0; i < tests_count; ++i)
		if (!do_test(tests[i], TEST_DECODE)) return 1;
	std::cout << "OK!" << std::endl;
	return 0;
}

