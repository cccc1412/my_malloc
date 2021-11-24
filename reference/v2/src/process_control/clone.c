#include "syscall.h"
#include "clone.h"

long clone(unsigned long flags, void *stack, int *parent_tid, int *child_tid, unsigned long tls)
{
	int ret = (int)SYSCALL5(56, flags, stack, parent_tid, child_tid, tls);
	return ret;
}
