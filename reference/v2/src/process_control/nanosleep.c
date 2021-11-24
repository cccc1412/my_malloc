#include "time.h"
#include "errno.h"

int nanosleep(const struct timespec *req, struct timespec *rem)
{
	int ret = clock_nanosleep(CLOCK_REALTIME, 0, req, rem);
	if(ret != 0)
	{
		errno = ret;
		return -1;
	}

	return 0;
}
