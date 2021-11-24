#ifndef THREADS_H
#define THREADS_H

#include "unistd.h"

typedef unsigned long thrd_t;
typedef int (*thrd_start_t)(void*);

enum
{
  thrd_success,
  thrd_busy,
  thrd_error,
  thrd_nomem,
  thrd_timedout
};
  
int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);

thrd_t thrd_current(void);

void thrd_exit(int __res);

int thrd_join(thrd_t __thr, int *__res);

int thrd_detach(thrd_t __thr);

struct SLThreadControlBlock
{
	struct SLThreadControlBlock *pSelf;
	
	thrd_start_t ThreadFunc;
	void *ThreadArg;

	void *pStack;

	volatile pid_t ThreadID;

	int Result;

	long IsDetached;
};

#define STACK_SIZE (8 * 1024 * 1024)

#endif