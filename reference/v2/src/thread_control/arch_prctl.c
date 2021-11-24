#include "sys/prctl.h"
#include "syscall.h"

int arch_prctl(int code, unsigned long addr)
{
	int ret = (int)SYSCALL2(158, code, addr);
	return ret;
}
