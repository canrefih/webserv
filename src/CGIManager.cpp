#include "CGIManager.hpp"
#include "CGIEnvBuilder.hpp"
#include "HttpResponse.hpp"

#include <sstream>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>

CGIManager::CGIManager( void )
{
}

CGIManager::~CGIManager()
{
	std::map<int, CgiSession*>::iterator it = _sessions.begin();

	for (; it != _sessions.end(); ++it) // Server is shutting down with CGI scripts still running: kill them so we don't leak zombie processes
	{
		it->second->cgi->kill();
		delete it->second->cgi;
		delete it->second;
	}
}

bool	CGIManager::isCgiFd(int fd) const
{
	return (_fdToClient.find(fd) != _fdToClient.end());
}

// Removes the pollfd entry with the given fd, if any (no-op if fd is -1 or already gone)
void	CGIManager::removeFd(std::vector<pollfd> &pollFds, int fd)
{
	for (std::size_t i = 0; i < pollFds.size(); ++i)
	{
		if (pollFds[i].fd == fd)
		{
			pollFds.erase(pollFds.begin() + i);
			return;
		}
	}
}

// Finds the client's own pollfd entry and switches the events it is watched for
void	CGIManager::setClientEvents(std::vector<pollfd> &pollFds, int clientFd, short events)
{
	for (std::size_t i = 0; i < pollFds.size(); ++i)
	{
		if (pollFds[i].fd == clientFd)
		{
			pollFds[i].events = events;
			return;
		}
	}
}

/*
Launches the CGI script matched by RequestHandler::resolveCGI() for this
request and registers everything the poll() loop needs to drive it
asynchronously: the request body queued to be written to its stdin, and
its stdout queued to be read into the eventual response.
*/
bool	CGIManager::start(int clientFd, const HttpRequest &request, const ServerConfig &serverConfig,
						  const std::string &scriptPath, const std::string &interpreterPath,
						  std::vector<pollfd> &pollFds, std::string &immediateErrorResponse)
{
	std::vector<std::string> env = buildCGIEnv(request, serverConfig.getHost(), serverConfig.getPort());
	CGIHandler *cgi = new CGIHandler();

	cgi->setup(scriptPath, interpreterPath, env);

	if (!cgi->start())
	{
		delete cgi;

		HttpResponse response;

		response.setStatus(502, "Bad Gateway");
		response.setBody("Bad Gateway\n");
		response.setContentType("text/plain");
		immediateErrorResponse = response.toString();
		return (false);
	}

	CgiSession *session = new CgiSession();

	session->cgi = cgi;
	session->body = request.getBody();
	session->bodySent = 0;
	session->start = time(NULL);
	session->keepAlive = (request.getHeader("Connection") != "close");
	_sessions[clientFd] = session;

	struct pollfd pfd;

	pfd.revents = 0;

	if (session->body.empty())
		cgi->closeStdinFd();
	else
	{
		pfd.fd = cgi->getStdinFd();
		pfd.events = POLLOUT;
		pollFds.push_back(pfd);
		_fdToClient[cgi->getStdinFd()] = clientFd;
	}

	pfd.fd = cgi->getStdoutFd();
	pfd.events = POLLIN;
	pollFds.push_back(pfd);
	_fdToClient[cgi->getStdoutFd()] = clientFd;

	return (true);
}

/*
Drives one CGI pipe fd that poll() reported ready: writes as much of the
pending request body as the pipe accepts (POLLOUT), or reads whatever the
script has written so far (POLLIN/POLLHUP), all without blocking. EOF on
read means the script is done producing output, which triggers finish().
*/
void	CGIManager::handleEvent(int fd, short revents, std::vector<pollfd> &pollFds)
{
	std::map<int, int>::iterator fdIt = _fdToClient.find(fd);

	if (fdIt == _fdToClient.end())
		return;

	int clientFd = fdIt->second;
	CgiSession *session = _sessions[clientFd];

	if (revents & POLLOUT)
	{
		ssize_t bytesWritten = write(
			fd,
			session->body.c_str() + session->bodySent,
			session->body.size() - session->bodySent);

		if (bytesWritten < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return; // Pipe not ready yet, retry on the next poll() tick

			abort(clientFd, session, pollFds, 500, "Internal Server Error");
			return;
		}

		session->bodySent += bytesWritten;

		if (session->bodySent == session->body.size())
		{
			session->cgi->closeStdinFd();
			_fdToClient.erase(fd);
			removeFd(pollFds, fd);
		}
		return;
	}

	if (revents & (POLLIN | POLLHUP))
	{
		char buffer[4096];
		ssize_t bytesRead = read(fd, buffer, sizeof(buffer));

		if (bytesRead < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return; // Nothing to read yet, retry on the next poll() tick

			abort(clientFd, session, pollFds, 500, "Internal Server Error");
			return;
		}

		if (bytesRead == 0) // EOF: the script closed its stdout, its output is complete
		{
			session->cgi->closeStdoutFd();
			_fdToClient.erase(fd);
			removeFd(pollFds, fd);
			finish(clientFd, session, pollFds);
			return;
		}

		session->output.append(buffer, bytesRead);
	}
}

/*
Turns the raw CGI output (CGI/1.1: header lines, a blank line, then the
body) into an HttpResponse, the same way a normal static-file response
would be queued. A "Status:" header overrides the default 200 OK. If the
script exited abnormally without writing anything, that's reported as a
502 instead of forwarding an empty 200 OK.
*/
void	CGIManager::finish(int clientFd, CgiSession *session, std::vector<pollfd> &pollFds)
{
	int exitCode = 0;
	int reaped = session->cgi->tryWait(exitCode); // Best-effort reap: stdout is already closed so the script is done or about to be

	HttpResponse response;

	if (reaped > 0 && exitCode != 0 && session->output.empty())
	{
		response.setStatus(502, "Bad Gateway");
		response.setBody("Bad Gateway\n");
		response.setContentType("text/plain");
		response.setHeader("Connection", "close");
		session->keepAlive = false;
	}
	else
	{
		std::string &output = session->output;
		std::size_t headerEnd = output.find("\r\n\r\n");
		std::size_t separatorLen = 4;

		if (headerEnd == std::string::npos)
		{
			headerEnd = output.find("\n\n");
			separatorLen = 2;
		}

		std::string headersBlob = (headerEnd == std::string::npos) ? "" : output.substr(0, headerEnd);
		std::string body = (headerEnd == std::string::npos) ? output : output.substr(headerEnd + separatorLen);

		response.setStatus(200, "OK");
		response.setContentType("text/html");

		std::istringstream headerStream(headersBlob);
		std::string line;

		while (std::getline(headerStream, line))
		{
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);

			std::size_t colon = line.find(':');

			if (colon == std::string::npos)
				continue;

			std::string name = line.substr(0, colon);
			std::string value = line.substr(colon + 1);

			while (!value.empty() && value[0] == ' ')
				value.erase(0, 1);

			if (name == "Status")
			{
				int code = std::atoi(value.c_str());
				std::string text = value;
				std::size_t space = value.find(' ');

				if (space != std::string::npos)
					text = value.substr(space + 1);

				response.setStatus(code, text);
			}
			else if (name == "Content-Type")
				response.setContentType(value);
			else
				response.setHeader(name, value);
		}

		response.setBody(body);
		response.setHeader("Connection", session->keepAlive ? "keep-alive" : "close");
	}

	CgiReadyResponse ready;

	ready.clientFd = clientFd;
	ready.response = response.toString();
	ready.keepAlive = session->keepAlive;
	_ready.push_back(ready);

	setClientEvents(pollFds, clientFd, POLLOUT);

	delete session->cgi;
	delete session;
	_sessions.erase(clientFd);
}

// Common cleanup for a CGI execution that failed or timed out: kill the child, drop its pipes, and answer the client with an error instead of leaving it hanging
void	CGIManager::abort(int clientFd, CgiSession *session, std::vector<pollfd> &pollFds, int statusCode, const std::string &statusText)
{
	session->cgi->kill();
	_fdToClient.erase(session->cgi->getStdinFd());
	_fdToClient.erase(session->cgi->getStdoutFd());
	removeFd(pollFds, session->cgi->getStdinFd());
	removeFd(pollFds, session->cgi->getStdoutFd());

	HttpResponse response;

	response.setStatus(statusCode, statusText);
	response.setBody(statusText + "\n");
	response.setContentType("text/plain");
	response.setHeader("Connection", "close");

	CgiReadyResponse ready;

	ready.clientFd = clientFd;
	ready.response = response.toString();
	ready.keepAlive = false;
	_ready.push_back(ready);

	setClientEvents(pollFds, clientFd, POLLOUT);

	delete session->cgi;
	delete session;
	_sessions.erase(clientFd);
}

// Called once per poll() tick: kills any CGI script that has been running longer than CGI_TIMEOUT_SECONDS
void	CGIManager::checkTimeouts(std::vector<pollfd> &pollFds)
{
	static const time_t CGI_TIMEOUT_SECONDS = 30;
	time_t now = time(NULL);
	std::map<int, CgiSession*>::iterator it = _sessions.begin();

	while (it != _sessions.end())
	{
		int clientFd = it->first;
		CgiSession *session = it->second;

		++it; // advance before a possible abort() erases the current entry

		if (now - session->start > CGI_TIMEOUT_SECONDS)
			abort(clientFd, session, pollFds, 504, "Gateway Timeout");
	}
}

// The client disconnected while a CGI was still running for it: kill it and drop its pipes, no response needed since there's no one left to send it to
void	CGIManager::abortForClient(int clientFd, std::vector<pollfd> &pollFds)
{
	std::map<int, CgiSession*>::iterator it = _sessions.find(clientFd);

	if (it == _sessions.end())
		return;

	CgiSession *session = it->second;

	session->cgi->kill();
	removeFd(pollFds, session->cgi->getStdinFd());
	removeFd(pollFds, session->cgi->getStdoutFd());
	_fdToClient.erase(session->cgi->getStdinFd());
	_fdToClient.erase(session->cgi->getStdoutFd());

	delete session->cgi;
	delete session;
	_sessions.erase(it);
}

bool	CGIManager::popReady(int &clientFd, std::string &response, bool &keepAlive)
{
	if (_ready.empty())
		return (false);

	CgiReadyResponse &front = _ready.front();

	clientFd = front.clientFd;
	response = front.response;
	keepAlive = front.keepAlive;

	_ready.erase(_ready.begin());
	return (true);
}
