#include "unistd.h"
#include "syscall.h"

off_t lseek(int fd, off_t offset, int whence)
{	
	long ret = SYSCALL3(8, fd, offset, whence);
    return ret;
}
