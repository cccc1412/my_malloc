#include "threads.h"
#include "clone.h"
#include "sys/mman.h"
#include "errno.h"
#include "unistd.h"

void RunThread(void *arg)
{
	struct SLThreadControlBlock *pTCB = (struct SLThreadControlBlock *)arg;

	thrd_exit(pTCB->ThreadFunc(pTCB->ThreadArg));
}

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{	
	char *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	if(stack == MAP_FAILED)
		return thrd_error;

	struct SLThreadControlBlock *pTCB = (struct SLThreadControlBlock *)(stack + STACK_SIZE - sizeof(struct SLThreadControlBlock));

	pTCB->pSelf = pTCB;

	pTCB->ThreadFunc = func;
	pTCB->ThreadArg = arg;
	pTCB->pStack = stack;

	pTCB->IsDetached = 0;
	pTCB->Result = 0;
	pTCB->ThreadID = 0;
	
	unsigned long *stack_top = (unsigned long *)pTCB - 1;

	*stack_top = (unsigned long)pTCB;
	
	stack_top--;

	*stack_top = (unsigned long)RunThread;

	unsigned long flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_SYSVSEM |
						  CLONE_THREAD | CLONE_SETTLS |
						  CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID;

	unsigned long tid_addr = (unsigned long)(&(pTCB->ThreadID));
	unsigned long tcb_addr = (unsigned long)pTCB;

	asm goto("movl $56, %%eax\n\t"
             "movq %0, %%rdi\n\t"
             "movq %1, %%rsi\n\t"
             "movq %2, %%rdx\n\t"
             "movq %3, %%r10\n\t"
             "movq %4, %%r8\n\t"
             "syscall\n\t"
             "cmpl $0, %%eax\n\t"
			 "js %l5\n\t"
			 "test %%rax, %%rax\n\t"
			 "jne %l6\n\t"
			 "xor %%rbp, %%rbp\n\t"
			 "pop %%rax\n\t"
			 "pop %%rdi\n\t"
			 "callq *%%rax"
             :
             :"m"(flags), "m"(stack_top), "m"(tid_addr), "m"(tid_addr), "m"(tcb_addr)
             :"cc", "rcx", "rax", "memory", "rdi", "rsi", "rdx", "r10", "r8", "r11"
             :lerror, lparent);

lerror:
	asm volatile("movl %%eax, %0"
				 :"=m"(errno));

	munmap(stack, STACK_SIZE);
	return thrd_error;

lparent:	
	*thr = (thrd_t)pTCB;

	return thrd_success;
}
