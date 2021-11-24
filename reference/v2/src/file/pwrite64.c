#include "unistd.h"
#include "syscall.h"

ssize_t pwrite64(int fd, void *buf, size_t count, off_t offset)
{	
	long ret = SYSCALL4(18, fd, buf, count, offset);
    return ret;
}
