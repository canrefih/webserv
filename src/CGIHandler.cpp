#include "CGIHandler.hpp"
#include "Signal.hpp"
#include <fcntl.h>
#include <cerrno>
#include <cstdlib>
#include <csignal>

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

/*
Computes the prefix that leads from "dir" back to the server's working directory,
so that a relative path keeps pointing to the same file after chdir(dir).
  "./www/cgi-bin" -> "../../"      "/abs/dir" -> "$PWD/"
Returns false when it can't be computed (".." in dir, or absolute dir without $PWD).
*/
static bool	pathBackFrom( const std::string &dir, std::string &back )
{
	back.clear();
	if (dir[0] == '/')
	{
		const char *pwd = std::getenv("PWD");

		if (pwd == NULL || pwd[0] != '/')
			return (false);
		back = std::string(pwd) + "/";
		return (true);
	}

	std::size_t start = 0;

	while (start <= dir.size())
	{
		std::size_t end = dir.find('/', start);

		if (end == std::string::npos)
			end = dir.size();

		std::string part = dir.substr(start, end - start);

		if (part == "..")
			return (false);
		if (!part.empty() && part != ".")
			back += "../";
		start = end + 1;
	}
	return (true);
}

CGIHandler::CGIHandler( void ) : pid(-1), running(false)
{
	for (int i = 0; i < 4; i++)
		fd[i] = -1; // -1 = closed: lets the destructor tell which pipe ends are still open
	std::cout << "CGIHandler created." << std::endl;
}

CGIHandler::~CGIHandler()
{
	closeStdinFd(); // A CGI aborted early (timeout, error, client gone) still has its pipe ends open: don't leak them
	closeStdoutFd();
	std::cout << "CGIHanlder destoyed. " << std::endl;
}

/*
Fills argv/envp (and the vectors that own the underlying strings)
with the values needed to launch the CGI script via execve()..
*/
void	CGIHandler::setup(const std::string &scriptPath, const std::string &interpreterPath, const std::vector<std::string> &env)
{
	_argv.clear();
    _envp.clear();
    _tmps.clear();
    _envTmps.clear();

	_scriptPath = scriptPath;
	_scriptDir.clear();
SI
	std::string interpreter = interpreterPath;
	std::string script = scriptPath;
	std::size_t slash = scriptPath.find_last_of('/');

	if (slash != std::string::npos)
	{
		std::string dir = (slash == 0) ? "/" : scriptPath.substr(0, slash);
		bool interpreterIsRelative = !interpreter.empty() && interpreter[0] != '/' && interpreter.find('/') != std::string::npos;
		std::string back;
		bool canChdir = true;

		if (interpreterIsRelative)
			canChdir = pathBackFrom(dir, back);
		if (canChdir)
		{
			_scriptDir = dir;
			script = scriptPath.substr(slash + 1);
			if (interpreterIsRelative)
				interpreter = back + interpreter;
		}
	}

	_tmps.push_back(interpreter);
	_tmps.push_back(script);
	for(std::size_t i = 0; i < _tmps.size(); i++)
		_argv.push_back(_tmps[i].c_str());
	_argv.push_back(NULL);
	_envTmps = env;
	for (std::size_t i = 0; i < _envTmps.size(); i++)
    	std::cerr << "CGI ENV: " << _envTmps[i] << std::endl;
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
		closeAllPipes();
		return (0);
	}
	if (fd[0] >= MAX_FD || fd[1] >= MAX_FD || fd[2] >= MAX_FD || fd[3] >= MAX_FD) // Over the limit other CGI children rely on to close inherited fds
	{
		std::cerr << "Too many open file descriptors, CGI refused." << std::endl;
		closeAllPipes();
		return (0);
	}
	if (!setNonBlocking(fd[1]) || !setNonBlocking(fd[2]))
	{
		std::cerr << strerror(errno) << " Failed to set pipes non-blocking." << std::endl;
		closeAllPipes();
		return (0);
	}
	return (1);
}

/*
Redirects the CGI's stdin/stdout onto the pipe ends it needs
(read end of pipe_in -> stdin, write end of pipe_out -> stdout),
closes every fd it doesn't need, then replaces itself with the
CGI interpreter via execve(). Never returns on success; if execve()
fails, logs the error and exits (killing only the child, not the server).
*/
void	CGIHandler::childProcess( void )
{
	if (dup2(fd[0], STDIN_FILENO) == -1)
	{
		std::cerr << "dup2 stdin failed: "
				<< strerror(errno) << std::endl;
		_exit(1);
	}

	if (dup2(fd[3], STDOUT_FILENO) == -1)
	{
		std::cerr << "dup2 stdout failed: "
				<< strerror(errno) << std::endl;
		_exit(1);
	}
	close(fd[0]);
	close(fd[1]);
	close(fd[2]);
	close(fd[3]);

	for (int i = 3; i < MAX_FD; i++) // fork() copied every fd the server had open (listening sockets, other clients, other CGI pipes); the child must not hold onto any of them. The server never uses a fd >= MAX_FD.
	{
		if (i == fd[0] || i == fd[1] || i == fd[2] || i == fd[3]) // Our own pipe ends were already closed just above
			continue;
		close(i);
	}

	if (!_scriptDir.empty() && chdir(_scriptDir.c_str()) == -1) // Run the CGI in its own directory (relative file access)
	{
		std::cerr << "chdir failed: " << _scriptDir << std::endl;
		_exit(1);
	}

	execve(_argv[0], const_cast<char**>(&_argv[0]), const_cast<char**>(&_envp[0]));
	std::cerr << "execve failed: "
			<< strerror(errno) << std::endl;
	_exit(errno);
}

/*
Entry point: validates argv, sets up the pipes, forks, then
dispatches to childProcess() and returns immediately in the parent
(the server). Never blocks: the caller is responsible for driving
fd[1]/fd[2] through poll() and reaping the child via tryWait().
*/
bool	CGIHandler::start( void )
{
	if (running)
    {
        std::cerr << "CGI is already running." << std::endl;
        return false;
    }
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
		closeAllPipes();
		return (false);
	}
	if (pid == 0)
		childProcess();
	close(fd[0]); // Child-side ends, only the child uses them
	close(fd[3]);
	fd[0] = -1;
	fd[3] = -1;
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

// Closes every pipe end still open and marks them -1 (used on setup failures)
void	CGIHandler::closeAllPipes( void )
{
	for (int i = 0; i < 4; i++)
	{
		if (fd[i] != -1)
		{
			close(fd[i]);
			fd[i] = -1;
		}
	}
}

pid_t	CGIHandler::getPid( void ) const
{
	return (pid);
}

bool	CGIHandler::isRunning( void ) const
{
	return (running);
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
	if (pid <= 0)
    {
        exitCode = -256;
        return -1;
    }

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

/*
Force-terminates the child (e.g. when it has run past the server's
CGI timeout). Only sends the signal while we still believe the
child is running; tryWait() still needs to be called afterwards to
reap it once waitpid() reports it as exited.
*/
void	CGIHandler::kill( void )
{
	if (running)
	{
		if (::kill(pid, SIGKILL) == -1)
			std::cerr << "Failed to kill CGI: "
					<< strerror(errno) << std::endl;
	}
}
