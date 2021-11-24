#include "threads.h"
#include "sys/mman.h"
#include "unistd.h"

int thrd_join(thrd_t __thr, int *__res)
{
	struct SLThreadControlBlock *pTCB = (struct SLThreadControlBlock *)__thr;

	if(pTCB->IsDetached)
		return thrd_error;

	int *pfutex = &(pTCB->ThreadID);
	int val = pTCB->ThreadID;
	
	while(val != 0)
	{
		futex_wait(pfutex, val);

		val = pTCB->ThreadID;
	}

	if(__res)
		*__res = pTCB->Result;

	munmap(pTCB->pStack, STACK_SIZE);

	return thrd_success;
}
