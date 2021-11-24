#include "unistd.h"
#include "syscall.h"

ssize_t pread64(int fd, void *buf, size_t count, off_t offset)
{	
	long ret = SYSCALL4(17, fd, buf, count, offset);
    return ret;
}
