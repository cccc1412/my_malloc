#ifndef WAIT_H
#define WAIT_H

#include "types.h"

#define WIFEXITED(status) (((status) & 0x7f) == 0)
#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)

#define WNOHANG 1

struct rusage;
pid_t wait4(pid_t pid, int *wstatus, int options, struct rusage *rusage);

pid_t waitpid(pid_t pid, int *wstatus, int options);

pid_t wait(int *wstatus);

#endif
