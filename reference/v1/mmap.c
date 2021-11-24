#include "syscall.h"
#include "stddef.h"

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{	
	void* paddr = (void *)SYSCALL6(9, addr, length, prot, flags, fd, offset);
    return paddr;
}
