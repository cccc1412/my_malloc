#include "unistd.h"
#include "syscall.h"

int close(int fd)
{	
	int ret = (int)SYSCALL1(3, fd);
    return ret;
}
