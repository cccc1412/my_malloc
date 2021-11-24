#include "unistd.h"
#include "syscall.h"

ssize_t write(int fd, const void *buf, size_t count)
{	
	long ret = SYSCALL3(1, fd, buf, count);
    return ret;
}
