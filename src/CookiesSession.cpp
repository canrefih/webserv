#include "CookiesSession.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstddef>

static const int	SESSION_MAX_AGE = 3600;
static const size_t	SESSION_ID_BYTES = 16;
static const size_t	SESSION_MAX_COUNT = 10000; // Upper bound so clients that never send their cookie back (e.g. siege) can't grow memory forever

CookiesSession::CookiesSession() : _maxAge(SESSION_MAX_AGE)
{
}

CookiesSession::~CookiesSession()
{
}

std::string	CookiesSession::generateSessionId( void ) const
{
	unsigned char	buffer[SESSION_ID_BYTES];
	std::ifstream	urandom("/dev/urandom", std::ios::binary);

	if (!urandom)
		return ("");
	urandom.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
	if (!urandom)
		return ("");

	std::ostringstream	oss;
	oss << std::hex << std::setfill('0');
	for (size_t i = 0; i < sizeof(buffer); ++i)
		oss << std::setw(2) << static_cast<unsigned int>(buffer[i]);
	return (oss.str());
}

std::string	CookiesSession::extractSessionId( const HttpRequest &request ) const
{
	const std::string	&cookieHeader = request.getHeader("Cookie");
	const std::string	key = "session_id=";
	size_t				pos = cookieHeader.find(key);

	if (pos == std::string::npos)
		return ("");
	pos += key.size();

	size_t	end = cookieHeader.find(';', pos);
	return (cookieHeader.substr(pos, end == std::string::npos ? std::string::npos : end - pos));
}

bool	CookiesSession::isValid( const std::string &id )
{
	if (id.empty())
		return (false);

	std::map<std::string, std::time_t>::iterator	it = _sessions.find(id);

	if (it == _sessions.end())
		return (false);
	if (it->second < std::time(NULL))
	{
		_sessions.erase(it);
		return (false);
	}
	return (true);
}

/*
Called before storing every new session. Sessions all live _maxAge seconds, so
_order (creation order) is also expiry order: drop expired sessions from the front,
and while the table is full drop the oldest one. Each id is popped once, so this
costs O(1) on average instead of scanning the whole table.
*/
void	CookiesSession::makeRoom( void )
{
	std::time_t	now = std::time(NULL);

	while (!_order.empty())
	{
		std::map<std::string, std::time_t>::iterator	it = _sessions.find(_order.front());
		bool	gone = (it == _sessions.end()); // already erased by isValid()

		if (!gone && it->second >= now && _sessions.size() < SESSION_MAX_COUNT)
			break ;
		if (!gone)
			_sessions.erase(it);
		_order.pop_front();
	}
}

std::string	CookiesSession::getCreateSession( const HttpRequest &request, HttpResponse &response )
{
	std::string	id = extractSessionId(request);

	if (isValid(id))
		return (id);

	id = generateSessionId();

	while (id.empty() || _sessions.find(id) != _sessions.end())
	{
		id = generateSessionId();
	}
	makeRoom();
	_sessions[id] = std::time(NULL) + _maxAge;
	_order.push_back(id);

	std::ostringstream	cookie;
	cookie << "session_id=" << id << "; Path=/; HttpOnly; Max-Age=" << _maxAge;
	response.setHeader("Set-Cookie", cookie.str());

	return (id);
}
