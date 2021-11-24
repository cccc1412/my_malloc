#include "threads.h"

int thrd_detach(thrd_t __thr)
{
	struct SLThreadControlBlock *pTCB = (struct SLThreadControlBlock *)__thr;

	pTCB->IsDetached = 1;

	return thrd_success;
}
