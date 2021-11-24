#include "unistd.h"
#include "errno.h"

pid_t __attribute__((naked)) vfork()
{
	asm goto("movq (%%rsp), %%rdi\n\t"
			 "movl $58, %%eax\n\t"
			 "syscall\n\t"
			 "cmpl $0, %%eax\n\t"
			 "js %l1\n\t"
		 	 "test %%rax, %%rax\n\t"
			 "je %l0\n\t"
			 "movq %%rdi, (%%rsp)\n\t"
			 "retq"
			 :
			 :
			 :
			 :child, error);

child:
	asm volatile("retq");

error:
	asm volatile("movl %%eax, %0\n\t"
				 "retq"
				 :"=m"(errno));	
}
