#include "time.h"
#include "syscall.h"

int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain)
{
	int ret = (int)SYSCALL4(230, clock_id, flags, request, remain);

	if(ret == -1)
		return errno;
	else
		return 0;
}
