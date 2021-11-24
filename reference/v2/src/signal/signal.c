#include "signal.h"
#include "errno.h"

int sigemptyset(sigset_t *set)
{
	int __cnt = _SIGSET_NWORDS;

	sigset_t *__set = set;

	 while (--__cnt >= 0) 
	 	__set->__val[__cnt] = 0;

	 return 0;
}

int sigaddset(sigset_t *set, int signum)
{
	unsigned long __mask = __sigmask(signum);

	unsigned long __word = __sigword (signum);

	set->__val[__word] |= __mask;

	return 0;
}

sighandler_t signal(int signum, sighandler_t handler)
{
	struct sigaction act, oact;

	if(handler == SIG_ERR || signum < 1 || signum >= NSIG)
	{   
		errno = EINVAL;
		return SIG_ERR;
	}

	act.__sigaction_handler.sa_handler = handler;

	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, signum);

	act.sa_flags = 0;

	if(sigaction(signum, &act, &oact) < 0)
		return SIG_ERR;

	return oact.__sigaction_handler.sa_handler;
}
