#include "syscall.h"
#include "unistd.h"

pid_t getpid(void)
{
	int ret = (int)SYSCALL0(39);
	return ret;
}

pid_t getppid(void)
{
	int ret = (int)SYSCALL0(110);
	return ret;
}
