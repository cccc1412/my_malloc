#include "sys/types.h"
#include "syscall.h"

pid_t gettid(void)
{
	int ret = (int)SYSCALL0(186);
	return ret;
}
