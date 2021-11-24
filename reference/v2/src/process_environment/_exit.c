#include "unistd.h"
#include "syscall.h"

void _exit(int status)
{	
	(void)SYSCALL1(60, status);
}
