#include "unistd.h"
#include "time.h"

unsigned int sleep(unsigned int seconds)
{
	struct timespec ts;
	ts.tv_sec = seconds;
	ts.tv_nsec = 0;

	if(nanosleep(&ts, &ts) != 0)
		return ts.tv_sec;

	return 0;
}