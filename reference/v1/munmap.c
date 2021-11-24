#include "syscall.h"
#include "stddef.h"

int munmap(void *addr, size_t length)
{	
	int ret = (int)SYSCALL2(11, addr, length);
    return ret;
}
