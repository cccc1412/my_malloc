#include "unistd.h"
#include "clone.h"
#include "signal.h"

pid_t fork(void)
{
	return clone(SIGCHLD, NULL, NULL, NULL, 0);
}
