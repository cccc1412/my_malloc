#include "sys/prctl.h"
#include "sys/types.h"
#include "stdlib.h"
#include "threads.h"

char **environ;

int main(int argc, char *argv[], char *envp[]);

void start_main(unsigned long *rsp)
{
	int argc = (int)(*rsp);

	rsp++;
	
	char **argv = (char **)rsp;

	environ = (char **)(rsp + argc + 1);

	struct SLThreadControlBlock tcb;
	
	tcb.pSelf = &tcb;
	tcb.ThreadFunc = NULL;
	tcb.ThreadArg = NULL;
	tcb.pStack = NULL;
	tcb.ThreadID = gettid();
	tcb.IsDetached = 1;
	tcb.Result = 0;

	arch_prctl(ARCH_SET_FS, (unsigned long)(&tcb));

	exit(main(argc, argv, environ));
}

void __attribute__((naked)) _start()
{
	asm volatile("movq %%rsp, %%rdi\n\t"
				 "callq start_main"
				 :::);
}
