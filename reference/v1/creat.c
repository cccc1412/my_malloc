#include "fcntl.h"
#include "syscall.h"

int creat(const char *pathname, mode_t mode)
{
	int ret = (int)SYSCALL2(85, pathname, mode);
    return ret;
}
