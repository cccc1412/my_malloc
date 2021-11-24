#include "unistd.h"
#include "syscall.h"

void exit_group(int status)
{	
	(void)SYSCALL1(231, status);
}
