#include "unistd.h"
#include "signal.h"
#include "sys/mman.h"
#include "errno.h"
#include "clone.h"

#define STACK_SIZE (16 * 1024)

pid_t vfork2(void (*child)(void *), void *arg)
{
	pid_t pid;
	
	char *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK | MAP_GROWSDOWN, -1, 0);
	if(stack == MAP_FAILED)
		return -1;

	unsigned long *stack_top = (unsigned long *)(stack + STACK_SIZE) - 1;
	
	*stack_top = (unsigned long)arg;

	stack_top--;

	*stack_top = (unsigned long)child;

	unsigned long flags = CLONE_VM | CLONE_VFORK | SIGCHLD;

	asm goto("movl $56, %%eax\n\t"
             "movq %0, %%rdi\n\t"
             "movq %1, %%rsi\n\t"
             "movq $0, %%rdx\n\t"
             "movq $0, %%r10\n\t"
             "movq $0, %%r8\n\t"
             "syscall\n\t"
             "cmpl $0, %%eax\n\t"
			 "js %l2\n\t"
			 "test %%rax, %%rax\n\t"
			 "jne %l3\n\t"
			 "xor %%rbp, %%rbp\n\t"
			 "pop %%rax\n\t"
			 "pop %%rdi\n\t"
			 "callq *%%rax\n\t"
             "jmp %l4"
             :
             :"m"(flags), "m"(stack_top)
             :"cc", "rcx", "rax", "memory", "rdi", "rsi", "rdx", "r10", "r8"
             :lerror, lparent, lchild);

lchild:
	_exit(0);

lerror:
	asm volatile("movl %%eax, %0"
				 :"=m"(errno));

	munmap(stack, STACK_SIZE);
	return -1;

lparent:	
	asm volatile("movl %%eax, %0"
				 :"=m"(pid));
	
	munmap(stack, STACK_SIZE);
	return pid;
}
