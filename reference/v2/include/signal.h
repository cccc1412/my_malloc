#ifndef SIGNAL_H
#define SIGNAL_H

#include "stddef.h"

#define SIGCHLD 17

#define __SIGRTMAX 64
#define _NSIG (__SIGRTMAX + 1)
#define NSIG _NSIG

typedef void (*__sighandler_t)(int);
typedef __sighandler_t sighandler_t;

#define SIG_ERR  ((__sighandler_t) -1) 
#define SIG_DFL  ((__sighandler_t)  0) 
#define SIG_IGN  ((__sighandler_t)  1) 

#define _SIGSET_NWORDS (1024 / (8 * sizeof (unsigned long)))

typedef struct 
{
    unsigned long __val[_SIGSET_NWORDS];
}__sigset_t;

typedef __sigset_t sigset_t;

#define __sigmask(sig) (((unsigned long) 1) << (((sig) - 1) % (8 * sizeof (unsigned long))))
#define __sigword(sig) (((sig) - 1) / (8 * sizeof (unsigned long)))

int sigemptyset(sigset_t *set);
int sigaddset(sigset_t *set, int signum);

#define SA_RESTORER 0x04000000

struct sigaction 
{
    union 
	{
        __sighandler_t sa_handler;
        void (*sa_sigaction)(int, void *, void *);
    } __sigaction_handler;
	
    __sigset_t sa_mask;
	
    int sa_flags;
	
    void (*sa_restorer)(void);
};

struct kernel_sigaction 
{
    __sighandler_t k_sa_handler;
    unsigned long sa_flags;
    void (*sa_restorer)(void);
    sigset_t sa_mask;
};

sighandler_t signal(int signum, sighandler_t handler);
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
int rt_sigaction(int signum, const struct kernel_sigaction *act, struct kernel_sigaction *oldact, size_t sigsetsize);

#endif
