#include "unistd.h"
#include "syscall.h"

int execve(const char *pathname, char *const argv[], char *const envp[])
{
	int ret = (int)SYSCALL3(59, pathname, argv, envp);
	return ret;
}

int execv(const char *pathname, char *const argv[])
{
	return execve(pathname, argv, environ);
}
