#include "sys/wait.h"
#include "syscall.h"

pid_t wait4(pid_t pid, int *wstatus, int options, struct rusage *rusage)
{
	int ret = (int)SYSCALL4(61, pid, wstatus, options, rusage);
	return ret;
}