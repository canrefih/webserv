#include "URL.hpp"
#include <iostream>

struct TestData {
	std::string input;
	std::string expected;
};

std::ostream& operator<<(std::ostream& out, TestData data)
{
	out << "{" << std::endl
		<< "input:    " << data.input << std::endl
		<< "expected: " << data.expected << std::endl
		<< "}";
	return out;
}

#define EXPECT_OK true
#define EXPECT_ERR false

struct CreateTestData {
	std::string input;
	bool        expect_ok;
	std::string path;
	std::string host;
	std::string query;
};

std::ostream& operator<<(std::ostream& out, CreateTestData data)
{
	out << "{" << std::endl
		<< "input:     " << data.input << std::endl
		<< "expect_ok: " << (data.expect_ok ? "true" : "false") << std::endl
		<< "path:      " << data.path << std::endl
		<< "host:      " << data.host << std::endl
		<< "query:     " << data.query << std::endl
		<< "}";
	return out;
}

enum TestType {
	TEST_ENCODE,
	TEST_DECODE_OK,
	TEST_DECODE_ERR,
	TEST_NORMALIZE_OK,
	TEST_NORMALIZE_ERR,
};

bool do_test(TestData data, TestType test_type)
{
	std::string result;

	switch (test_type)
	{
		case TEST_ENCODE:
		{
			result = URL::encode(data.input);
		} break;
		case TEST_DECODE_OK:
		{
			std::pair<std::string, bool> decode_res = URL::decode(data.input);
			if (decode_res.second == false)
			{
				std::cout << std::endl << "Error!" << std::endl << std::endl
					<< "decode test failed on valid input: " << data << std::endl;
				return false;
			}
			result = decode_res.first;
		} break;
		case TEST_DECODE_ERR:
		{
			std::pair<std::string, bool> decode_res = URL::decode(data.input);
			if (decode_res.second == true)
			{
				std::cout << std::endl << "Error!" << std::endl << std::endl
					<< "decode should have failed on: " << data.input << std::endl;
				return false;
			}
			return true;
		} break;
		case TEST_NORMALIZE_OK:
		{
			std::pair<std::string, bool> normalize_res = URL::normalize(data.input);
			if (normalize_res.second == false)
			{
				std::cout << std::endl << "Error!" << std::endl << std::endl
					<< "normalize test failed on valid input: " << data << std::endl;
				return false;
			}
			result = normalize_res.first;
		} break;
		case TEST_NORMALIZE_ERR:
		{
			std::pair<std::string, bool> normalize_res = URL::normalize(data.input);
			if (normalize_res.second == true)
			{
				std::cout << std::endl << "Error!" << std::endl << std::endl
					<< "normalize should fail on out-of-scope '..': "
					<< data.input << std::endl;
				return false;
			}
			return true;
		} break;
	}
	if (result != data.expected)
	{
		std::cout << std::endl << "Error!" << std::endl << std::endl
			<< "test data: " << data << std::endl
			<< "result:\t\t" << result << std::endl
			<< "expected:\t" << data.expected << std::endl << std::endl;
		return false;
	}
	return true;
}

bool do_create_test(CreateTestData data)
{
	std::pair<URL, bool> create_res = URL::createFromRequestTarget(data.input);

	if (create_res.second != data.expect_ok)
	{
		std::cout << std::endl << "Error!" << std::endl << std::endl
			<< "createFromRequestTarget should have "
			<< (data.expect_ok ? "succeeded" : "failed") << " on: "
			<< data.input << std::endl;
		return false;
	}
	if (create_res.second == false)
		return true;

	URL url = create_res.first;

	if (url.getPath() != data.path
		|| url.getHost() != data.host
		|| url.getQuery() != data.query)
	{
		std::cout << std::endl << "Error!" << std::endl << std::endl
			<< "test data: " << data << std::endl
			<< "result:\tpath=\"" << url.getPath()
			<< "\" host=\"" << url.getHost()
			<< "\" query=\"" << url.getQuery() << "\"" << std::endl << std::endl;
		return false;
	}
	return true;
}

int main(void)
{
	TestData encode_tests[] = {
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

	TestData decode_err_tests[] = {
		{"%", ""},
		{"%1", ""},
		{"%GG", ""},
		{"hello%2", ""},
		{"a%zzb", ""},
		{"%2f%2", ""},
		{"100 % cat", ""},
	};

	TestData normalize_tests[] = {
		{"a/b", "a/b"},
		{"/a/b/c", "/a/b/c"},
		{"a/./b", "a/./b"},
		{"./a/b", "./a/b"},
		{"a/b/../c", "/a/c"},
		{"/a/b/../../c", "/c"},
		{"a/./b/../c", "/a/c"},
		{"a//b/./../c//d", "/a/c/d"},
		{"a/b/c", "a/b/c"},
		{"a/b/c/../../../", "/"},
		{"a/../c/../d/../basdasd/../", "/"},
		{"/", "/"},
	};

	TestData normalize_err_tests[] = {
		{"..", ""},
		{"../a", ""},
		{"a/../..", ""},
		{"/a/../../b", ""},
		{"a/b/../../../c", ""},
		{"a/b/../../c/../a/b/c/d/ef/../../../../../../", ""},
	};

	std::size_t count;

	std::cout << "Encoding tests\n";
	count = sizeof(encode_tests) / sizeof(TestData);
	for (std::size_t i = 0; i < count; ++i)
		if (!do_test(encode_tests[i], TEST_ENCODE)) return 1;
	std::cout << "OK!\n";

	// le décodage est l'inverse de l'encodage : input/expected inversés
	std::cout << "Decoding tests\n";
	count = sizeof(encode_tests) / sizeof(TestData);
	for (std::size_t i = 0; i < count; ++i)
	{
		TestData reversed = {encode_tests[i].expected, encode_tests[i].input};
		if (!do_test(reversed, TEST_DECODE_OK)) return 1;
	}
	std::cout << "OK!\n";

	std::cout << "Decoding error tests\n";
	count = sizeof(decode_err_tests) / sizeof(TestData);
	for (std::size_t i = 0; i < count; ++i)
		if (!do_test(decode_err_tests[i], TEST_DECODE_ERR)) return 1;
	std::cout << "OK!\n";

	std::cout << "Normalize tests\n";
	count = sizeof(normalize_tests) / sizeof(TestData);
	for (std::size_t i = 0; i < count; ++i)
		if (!do_test(normalize_tests[i], TEST_NORMALIZE_OK)) return 1;
	std::cout << "OK!\n";

	std::cout << "Normalize out-of-scope tests\n";
	count = sizeof(normalize_err_tests) / sizeof(TestData);
	for (std::size_t i = 0; i < count; ++i)
		if (!do_test(normalize_err_tests[i], TEST_NORMALIZE_ERR)) return 1;
	std::cout << "OK!" << std::endl;

	CreateTestData create_tests[] = {
		{"/", true, "/", "", ""},
		{"/index.html", true, "/index.html", "", ""},
		{"/a/b/c", true, "/a/b/c", "", ""},
		{"/search?q=webserv&lang=fr", true, "/search", "", "q=webserv&lang=fr"},
		{"/path?", true, "/path", "", ""},
		{"/?q=hello world", true, "/", "", "q=hello world"},
		// la query N'EST PAS décodée : les %XX restent bruts (destinée à QUERY_STRING du CGI)
		{"/search?q=a%20b%26c", true, "/search", "", "q=a%20b%26c"},
		{"/hello%20world", true, "/hello world", "", ""},
		{"/%41%42%43", true, "/ABC", "", ""},
		{"/caf%C3%A9", true, "/caf\xC3\xA9", "", ""},
		{"/a%2Fb", true, "/a/b", "", ""},
		{"/a/b/../c", true, "/a/c", "", ""},
		{"/a/b/../c?d=1", true, "/a/c", "", "d=1"},
		{"/a//b///c", true, "/a//b///c", "", ""},
		{"/a/./b", true, "/a/./b", "", ""},
		{"http://example.com/index.html", true, "/index.html", "example.com", ""},
		{"https://example.com:8080/a/b?x=1", true, "/a/b", "example.com:8080", "x=1"},
		{"http://example.com", true, "/", "example.com", ""},
		{"http://example.com/", true, "/", "example.com", ""},
	};

	CreateTestData create_err_tests[] = {
		{"", false, "", "", ""},
		{"index.html", false, "", "", ""},
		{"?q=1", false, "", "", ""},
		{"ftp://example.com/", false, "", "", ""},
		{"http:/example.com", false, "", "", ""},
		{"http", false, "", "", ""},
		{"/path#fragment", false, "", "", ""},
		{"http://example.com/p#f", false, "", "", ""},
		{"/a b", false, "", "", ""},
		{"/bad%zz", false, "", "", ""},
		{"/bad%2", false, "", "", ""},
		{"/a/../../b", false, "", "", ""},
		{"/../secret", false, "", "", ""},
		{"/%2e%2e/secret", false, "", "", ""},
		{"/%2E%2E/secret", false, "", "", ""},
	};

	std::cout << "CreateFromRequestTarget tests\n";
	count = sizeof(create_tests) / sizeof(CreateTestData);
	for (std::size_t i = 0; i < count; ++i)
		if (!do_create_test(create_tests[i])) return 1;
	std::cout << "OK!\n";

	std::cout << "CreateFromRequestTarget error tests\n";
	count = sizeof(create_err_tests) / sizeof(CreateTestData);
	for (std::size_t i = 0; i < count; ++i)
		if (!do_create_test(create_err_tests[i])) return 1;
	std::cout << "OK!" << std::endl;

	return 0;
}
