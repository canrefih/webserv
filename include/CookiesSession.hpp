#ifndef COOKIESSESSION_HPP
# define COOKIESSESSION_HPP

#include <string>
#include <map>
#include <deque>
#include <ctime>

class HttpRequest;
class HttpResponse;

class CookiesSession
{
private:
	std::map<std::string, std::time_t>	_sessions;
	std::deque<std::string>				_order; // session ids in creation order = expiry order (same Max-Age for all)
	int									_maxAge;
	std::string	generateSessionId( void ) const;
	std::string	extractSessionId( const HttpRequest &request ) const;
	void		makeRoom( void );

public:
	CookiesSession();
	~CookiesSession();
	std::string	getCreateSession( const HttpRequest &request, HttpResponse &response );

	bool	isValid( const std::string &id );
};

#endif
