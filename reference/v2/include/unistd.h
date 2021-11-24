#ifndef UNISTD_H
#define UNISTD_H

#include "stddef.h"
#include "sys/types.h"

extern char **environ;

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define EXIT_SUCCESS 0
#define EXIT_FAILURE -1

void _exit(int status);
void exit_group(int status);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

off_t lseek(int fd, off_t offset, int whence);

ssize_t read(int fd, void *buf, size_t count);

ssize_t write(int fd, const void *buf, size_t count);

int close(int fd);

ssize_t pread(int fd, void *buf, size_t count, off_t offset);
ssize_t pwrite(int fd, void *buf, size_t count, off_t offset);

ssize_t pread64(int fd, void *buf, size_t count, off_t offset);
ssize_t pwrite64(int fd, void *buf, size_t count, off_t offset);

int execve(const char *pathname, char *const argv[], char *const envp[]);

int execv(const char *pathname, char *const argv[]);

pid_t fork(void);

pid_t __attribute__((naked)) vfork();
pid_t vfork2(void (*child)(void *), void *arg);

pid_t getpid(void);

pid_t getppid(void);

unsigned int sleep(unsigned int seconds);

int futex_wait(int *uaddr, int val);

#endif