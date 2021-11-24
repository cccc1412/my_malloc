#include "signal.h"
#include "syscall.h"

int rt_sigaction(int signum, const struct kernel_sigaction *act, struct kernel_sigaction *oldact, size_t sigsetsize)
{
	int result = (int)SYSCALL4(13, signum, act, oldact, sigsetsize);
	return result;
}
