#ifndef TIME_H
#define TIME_H

typedef int clockid_t;

struct timespec 
{
	long tv_sec;
	long tv_nsec;
};

#define CLOCK_REALTIME 0

int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *request, struct timespec *remain);

int nanosleep(const struct timespec *req, struct timespec *rem);

#endif