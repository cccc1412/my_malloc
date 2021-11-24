#include "signal.h"
#include "syscall.h"
#include "errno.h"
#include "stddef.h"

extern void restorer();

asm("restorer:mov $15,%eax\n\t"
	 "syscall");

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
	if(signum < 1 || signum >= NSIG)
	{   
		errno = EINVAL;
		return -1;
	}

	struct kernel_sigaction kact, koact;

	if(act)
    {   
    	kact.k_sa_handler = act->__sigaction_handler.sa_handler;

		for(int i = 0; i < _SIGSET_NWORDS; i++)
			kact.sa_mask.__val[i] = act->sa_mask.__val[i];

		kact.sa_flags = act->sa_flags | SA_RESTORER;

		kact.sa_restorer = &restorer;
    }

	unsigned long sigsetsize = _NSIG / 8;

	int result = rt_sigaction(signum, act ? &kact : NULL, oldact ? &koact : NULL, sigsetsize);

	if(oldact && result >= 0)
    {   
    	oldact->__sigaction_handler.sa_handler = koact.k_sa_handler;

		for(int i = 0; i < _SIGSET_NWORDS; i++)
			oldact->sa_mask.__val[i] = koact.sa_mask.__val[i];
		
      	oldact->sa_flags = koact.sa_flags;
		
		oldact->sa_restorer = koact.sa_restorer;
    }

	return result;
}
