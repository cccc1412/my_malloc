#include "threads.h"
#include "unistd.h"

void thrd_exit(int __res)
{
	struct SLThreadControlBlock *pTCB = (struct SLThreadControlBlock *)thrd_current();

	if(!pTCB->IsDetached)
	{
		pTCB->Result = __res;
		_exit(__res);
	}

	unsigned long stack_addr = (unsigned long)(pTCB->pStack);

	asm volatile("movl %0, %%eax\n\t"
				 "movq %1, %%rdi\n\t"
				 "movq %2, %%rsi\n\t"
				 "syscall\n\t"
				 "movl %3, %%eax\n\t"
				 "movq $0, %%rdi\n\t"
				 "syscall"
				 :
				 :"i"(11), "m"(stack_addr), "i"(STACK_SIZE), "i"(60)
				 :"cc", "r11", "rax", "rdi", "rsi", "memory");
}