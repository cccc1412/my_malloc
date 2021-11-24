#include "fcntl.h"
#include "syscall.h"

int open(const char *pathname, int flags, ...)
{
	int mode = 0;

	if(flags & O_CREAT)
	{
		__builtin_va_list arg;
        __builtin_va_start(arg, flags);
		
        mode = __builtin_va_arg(arg, int);

		__builtin_va_end(arg);
	}

	int ret = (int)SYSCALL3(2, pathname, flags, mode);
    return ret;
}
