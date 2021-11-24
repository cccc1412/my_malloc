#ifndef UNISTD_H
#define UNISTD_H

#include "stddef.h"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define EXIT_SUCCESS 0
#define EXIT_FAILURE -1

void _exit(int status);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

off_t lseek(int fd, off_t offset, int whence);

ssize_t read(int fd, void *buf, size_t count);

ssize_t write(int fd, const void *buf, size_t count);

int close(int fd);

#endif