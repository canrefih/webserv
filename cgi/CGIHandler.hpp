#ifndef CGI_HPP
 #define CGI_HPP

#include <string>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <cstddef>
#include <sys/wait.h>
#include <string.h>

class CGIHandler
{
private:
	std::vector<const char*> _argv;
	std::vector<const char*> _envp;
	std::vector<std::string> _tmps;
	std::vector<std::string> _envTmps;
	std::string _scriptPath;

	int fd[4];
	pid_t pid;
	bool running;

	bool	setupPipes( void );
	void	childProcess( void );

public:
	CGIHandler( void );
	~CGIHandler();
	void	setup(const std::string &scriptPath, const std::string &interpreterPath, const std::vector<std::string> &env);

	bool	start( void );

	int		getStdinFd( void ) const;	//parent -> CGI (write request body here)
	int		getStdoutFd( void ) const;	//CGI -> parent (read response here)
	void	closeStdinFd( void );		//once the whole body has been sent
	void	closeStdoutFd( void );		//once EOF has been read

	int		tryWait( int &exitCode );
};


# endif
