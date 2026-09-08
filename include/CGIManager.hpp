#ifndef CGIMANAGER_HPP
#define CGIMANAGER_HPP

#include "CGIHandler.hpp"
#include "HttpRequest.hpp"
#include "ServerConfig.hpp"

#include <map>
#include <string>
#include <vector>
#include <poll.h>
#include <ctime>

/*
Tracks one in-flight CGI execution: the CGIHandler doing the fork/exec,
the request body still to be written to its stdin, the output read so
far from its stdout, and when it started (for the timeout check).
*/
struct CgiSession
{
	CGIHandler	*cgi;
	std::string	body;
	std::size_t	bodySent;
	std::string	output;
	time_t		start;
	bool		keepAlive;
};

// One finished (or aborted) CGI execution, waiting to be turned into a client write buffer by Server.
struct CgiReadyResponse
{
	int			clientFd;
	std::string	response;
	bool		keepAlive;
};

/*
Owns every in-flight CGI execution and everything needed to drive it
through poll() asynchronously. Server.cpp only has to: dispatch a
matched request to start(), route pollfds it doesn't recognize as its
own client sockets to isCgiFd()/handleEvent(), call checkTimeouts()
once per tick, and drain finished responses with popReady(). Keeping
this out of Server.cpp/Server.hpp (owned by another team member) avoids
merge conflicts on that shared file.
*/
class CGIManager
{
	private:
		std::map<int, CgiSession*>		_sessions;	// client fd -> in-flight session
		std::map<int, int>				_fdToClient;	// CGI stdin/stdout fd -> client fd
		std::vector<CgiReadyResponse>	_ready;		// finished/aborted results waiting to be picked up

		void	removeFd(std::vector<pollfd> &pollFds, int fd);
		void	setClientEvents(std::vector<pollfd> &pollFds, int clientFd, short events);
		void	finish(int clientFd, CgiSession *session, std::vector<pollfd> &pollFds);
		void	abort(int clientFd, CgiSession *session, std::vector<pollfd> &pollFds, int statusCode, const std::string &statusText);

	public:
		CGIManager();
		~CGIManager();

		bool	isCgiFd(int fd) const;

		// Launches the CGI script asynchronously. Returns false with immediateErrorResponse filled
		// if the launch itself failed (fork/pipe error) - nothing async was started in that case.
		bool	start(int clientFd, const HttpRequest &request, const ServerConfig &serverConfig,
					  const std::string &scriptPath, const std::string &interpreterPath,
					  std::vector<pollfd> &pollFds, std::string &immediateErrorResponse);

		void	handleEvent(int fd, short revents, std::vector<pollfd> &pollFds);
		void	checkTimeouts(std::vector<pollfd> &pollFds);
		void	abortForClient(int clientFd, std::vector<pollfd> &pollFds); // client disconnected while its CGI was still running

		bool	popReady(int &clientFd, std::string &response, bool &keepAlive);
};

#endif
