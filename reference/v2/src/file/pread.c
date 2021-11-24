#include "unistd.h"

ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{	
    return pread64(fd, buf, count, offset);
}
