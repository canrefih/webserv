#include "CGIHandler.hpp"
#include "Signal.hpp"
#include <fcntl.h>
#include <cerrno>
#include <cstdlib>

static bool	setNonBlocking( int fd )
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return (false);
	int ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if (ret == -1)
		return (false);
	return (true);
}

CGIHandler::CGIHandler( void ) : pid(-1), running(false)
{
	std::cout << "CGIHandler created." << std::endl;
}

CGIHandler::~CGIHandler()
{
	std::cout << "CGIHanlder destoyed. " << std::endl;
}

/*
Fills argv/envp (and the vectors that own the underlying strings)
with the values needed to launch the CGI script via execve()..
*/
void	CGIHandler::setup(const std::string &scriptPath, const std::string &interpreterPath, const std::vector<std::string> &env)
{
	_scriptPath = scriptPath;
	_tmps.push_back(interpreterPath);
	_tmps.push_back(scriptPath);
	for(std::size_t i = 0; i < _tmps.size(); i++)
		_argv.push_back(_tmps[i].c_str());
	_argv.push_back(NULL);
	_envTmps = env;
	for(std::size_t i = 0; i < _envTmps.size(); i++)
		_envp.push_back(_envTmps[i].c_str());
	_envp.push_back(NULL);


}

/*
Creates the two pipes used to talk to the CGI child process:
fd[0]/fd[1] = pipe_in  (read/write), server -> CGI, for the request body
fd[2]/fd[3] = pipe_out (read/write), CGI -> server, for the response
Returns false on failure, closing any pipe already opened to avoid leaks.
*/
bool	CGIHandler::setupPipes( void )
{
	if (pipe(fd) == -1)
	{
		std::cerr << strerror(errno) << std::endl;
		return (0);
	}
	if (pipe(fd + 2) == -1)
	{
		std::cerr << strerror(errno) << " On second pipe." << std::endl;
		close(fd[0]);
		close(fd[1]);
		return (0);
	}
	if (!setNonBlocking(fd[1]) || !setNonBlocking(fd[2]))
	{
		std::cerr << strerror(errno) << " Failed to set pipes non-blocking." << std::endl;
		close(fd[0]);
		close(fd[1]);
		close(fd[2]);
		close(fd[3]);
		return (0);
	}
	return (1);
}

/*
Runs only in the child process (pid == 0).
Redirects the CGI's stdin/stdout onto the pipe ends it needs
(read end of pipe_in -> stdin, write end of pipe_out -> stdout),
closes every fd it doesn't need, then replaces itself with the
CGI interpreter via execve(). Never returns on success; if execve()
fails, logs the error and exits (killing only the child, not the server).
*/
void	CGIHandler::childProcess( void )
{
	dup2(fd[0], 0);
	dup2(fd[3], 1);
	close(fd[0]);
	close(fd[1]);
	close(fd[2]);
	close(fd[3]);
	execve(_argv[0], const_cast<char**>(&_argv[0]), const_cast<char**>(&_envp[0]));
	std::cerr<< strerror(errno) << std::endl;
	exit(errno);
}

/*
Entry point: validates argv, sets up the pipes, forks, then
dispatches to childProcess() and returns immediately in the parent
(the server). Never blocks: the caller is responsible for driving
fd[1]/fd[2] through poll() and reaping the child via tryWait().
*/
bool	CGIHandler::start( void )
{
	if (_argv.size() == 0)
	{
		std::cerr << "Problem with argv[0]" << std::endl;
		return (false);
	}
	if (!setupPipes())
		return (false);
	pid = fork();
	if (pid == -1)
	{
		std::cerr << "Error pid." << std::endl;
		close(fd[0]); close(fd[1]); close(fd[2]); close(fd[3]);
		return (false);
	}
	if (pid == 0)
		childProcess();
	close(fd[0]);
	close(fd[3]);
	running = true;
	return (true);
}

int	CGIHandler::getStdinFd( void ) const
{
	return (fd[1]);
}

int	CGIHandler::getStdoutFd( void ) const
{
	return (fd[2]);
}

void	CGIHandler::closeStdinFd( void )
{
	if (fd[1] != -1)
	{
		close(fd[1]);
		fd[1] = -1;
	}
}

void	CGIHandler::closeStdoutFd( void )
{
	if (fd[2] != -1)
	{
		close(fd[2]);
		fd[2] = -1;
	}
}

/*
Non-blocking reap: uses WNOHANG so it can be called from the main
poll() loop (e.g. once EOF is read on fd[2]) without ever stalling
the server. Same status-decoding convention as before:
  >= 0 -> real CGI exit code (0-255)
  < 0  -> child was killed by a signal (value = -signal number)
  -256 -> unexpected/residual case (neither normal exit nor signal)
*/
int		CGIHandler::tryWait( int &exitCode )
{
	int		status = -1;
	pid_t	ret = waitpid(pid, &status, WNOHANG);

	if (ret == 0)
		return (0);
	if (ret == -1)
	{
		std::cerr << strerror(errno) << std::endl;
		return (-1);
	}
	running = false;
	g_serverRunning = false; // test with Signal.hpp for main loop
	if (WIFEXITED(status))
		exitCode = WEXITSTATUS(status);
	else if (WIFSIGNALED(status))
	{
		std::cerr << "CGI killed by signal " << WTERMSIG(status) << std::endl;
		exitCode = -WTERMSIG(status);
	}
	else
		exitCode = -256;
	return (1);
}
