#include "unistd.h"
#include "syscall.h"

ssize_t read(int fd, void *buf, size_t count)
{	
	long ret = SYSCALL3(0, fd, buf, count);
    return ret;
}
