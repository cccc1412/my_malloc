#include "unistd.h"

ssize_t pwrite(int fd, void *buf, size_t count, off_t offset)
{	
	return pwrite64(fd, buf, count, offset);
}
