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
	std::vector<const char*> argv;
	std::vector<const char*> envp;
	std::vector<std::string> tmps;
	std::vector<std::string> envTmps;
	std::string scriptPath;

	int fd[4];
	pid_t pid;
	bool running;

	bool	setupPipes( void );
	void	childProcess( void );

public:
	CGIHandler( void );
	~CGIHandler();
	void	setup( void );

	bool	start( void );

	int		getStdinFd( void ) const;	//parent -> CGI (write request body here)
	int		getStdoutFd( void ) const;	//CGI -> parent (read response here)
	void	closeStdinFd( void );		//once the whole body has been sent
	void	closeStdoutFd( void );		//once EOF has been read

	int		tryWait( int &exitCode );
};


# endif
