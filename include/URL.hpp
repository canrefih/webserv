#ifndef URL_HPP
#define URL_HPP

#include <string>
#include <utility>

class URL
{
	std::string _host;
	std::string _path;
	std::string _query;

public:
	URL();
	~URL();
	URL(const URL& other);
	URL& operator=(const URL& other);

	std::string getContent() const;
	const std::string& getHost() const;
	const std::string& getPath() const;
	const std::string& getQuery() const;

	static std::string encode(const std::string& raw);
	static std::pair<std::string, bool> decode(const std::string& s);
	static std::pair<std::string, bool> normalize(const std::string& s);

	static std::pair<URL, bool> createFromRequestTarget(const std::string& s);
};

#endif // URL_HPP
