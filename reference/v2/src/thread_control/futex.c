#include "unistd.h"
#include "syscall.h"

#define FUTEX_WAIT 0

struct timespec;

int futex(int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, int val3)
{
	int ret = (int)SYSCALL6(202, uaddr, futex_op, val, timeout, uaddr2, val3);
	return ret;
}

int futex_wait(int *uaddr, int val)
{
	return futex(uaddr, FUTEX_WAIT, val, NULL, NULL, 0);
}
