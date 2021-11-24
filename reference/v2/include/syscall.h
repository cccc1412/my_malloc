#ifndef SYSCALL_H
#define SYSCALL_H

#include "errno.h"

#define SYSCALL_CLOBBERS "cc", "r11", "rcx", "rax", "memory"

#define SYSCALL_ERROR_PROCESS(ret) 	\
	if(ret < 0 && ret > -4096)		\
	{								\
		errno = -ret;				\
		ret = -1;					\
	}

#define SYSCALL0(num) \
	({	\
		long ret; \
		asm volatile("movl %1, %%eax\n\t"	\
					 "syscall\n\t"			\
					 "movq %%rax, %0"		\
					 :"=m"(ret) 			\
				     :"i"(num)				\
					 :SYSCALL_CLOBBERS); 	\
											\
		SYSCALL_ERROR_PROCESS(ret)			\
											\
		ret;								\
	})

#define SYSCALL1(num, arg1) \
	({	\
		long ret; \
		asm volatile("movl %1, %%eax\n\t"	\
	 			 	 "movq %2, %%rdi\n\t"	\
				  	 "syscall\n\t"			\
				  	 "movq %%rax, %0"		\
				 	 :"=m"(ret)				\
				     :"i"(num),"m"(arg1)	\
				     :SYSCALL_CLOBBERS, "rdi");	\
											\
		SYSCALL_ERROR_PROCESS(ret)			\
											\
		ret;								\
	})

#define SYSCALL2(num, arg1, arg2) \
	({	\
		long ret; \
		asm volatile("movl %1, %%eax\n\t"	\
					 "movq %2, %%rdi\n\t"	\
					 "movq %3, %%rsi\n\t"	\
					 "syscall\n\t"			\
					 "movq %%rax, %0"		\
					 :"=m"(ret) 			\
					 :"i"(num),"m"(arg1),"m"(arg2)		\
					 :SYSCALL_CLOBBERS, "rdi", "rsi"); 	\
											\
		SYSCALL_ERROR_PROCESS(ret)			\
											\
		ret;								\
	})
	
#define SYSCALL3(num, arg1, arg2, arg3) \
	({	\
		long ret; \
		asm volatile("movl %1, %%eax\n\t"	\
					 "movq %2, %%rdi\n\t"	\
					 "movq %3, %%rsi\n\t"	\
					 "movq %4, %%rdx\n\t"	\
					 "syscall\n\t"			\
					 "movq %%rax, %0"		\
					 :"=m"(ret) 			\
					 :"i"(num),"m"(arg1),"m"(arg2), "m"(arg3)	\
					 :SYSCALL_CLOBBERS, "rdi", "rsi", "rdx");	\
											\
		SYSCALL_ERROR_PROCESS(ret)			\
											\
		ret;								\
	})

#define SYSCALL4(num, arg1, arg2, arg3, arg4) \
	({	\
		long ret; \
		asm volatile("movl %1, %%eax\n\t"	\
					 "movq %2, %%rdi\n\t"	\
					 "movq %3, %%rsi\n\t"	\
					 "movq %4, %%rdx\n\t"	\
					 "movq %5, %%r10\n\t"	\
					 "syscall\n\t"			\
					 "movq %%rax, %0"		\
					 :"=m"(ret) 			\
					 :"i"(num),"m"(arg1),"m"(arg2), "m"(arg3), "m"(arg4)	\
					 :SYSCALL_CLOBBERS, "rdi", "rsi", "rdx", "r10");		\
											\
		SYSCALL_ERROR_PROCESS(ret)			\
											\
		ret;								\
	})

#define SYSCALL5(num, arg1, arg2, arg3, arg4, arg5) \
	({	\
		long ret; \
		asm volatile("movl %1, %%eax\n\t"	\
					 "movq %2, %%rdi\n\t"	\
					 "movq %3, %%rsi\n\t"	\
					 "movq %4, %%rdx\n\t"	\
					 "movq %5, %%r10\n\t"	\
					 "movq %6, %%r8\n\t"	\
					 "syscall\n\t"			\
					 "movq %%rax, %0"		\
					 :"=m"(ret) 			\
					 :"i"(num),"m"(arg1),"m"(arg2), "m"(arg3), "m"(arg4), "m"(arg5)	\
					 :SYSCALL_CLOBBERS, "rdi", "rsi", "rdx", "r10", "r8");			\
											\
		SYSCALL_ERROR_PROCESS(ret)			\
											\
		ret;								\
	})

#define SYSCALL6(num, arg1, arg2, arg3, arg4, arg5, arg6) \
	({	\
		long ret; \
		asm volatile("movl %1, %%eax\n\t"	\
					 "movq %2, %%rdi\n\t"	\
					 "movq %3, %%rsi\n\t"	\
					 "movq %4, %%rdx\n\t"	\
					 "movq %5, %%r10\n\t"	\
					 "movq %6, %%r8\n\t"	\
					 "movq %7, %%r9\n\t"	\
					 "syscall\n\t"			\
					 "movq %%rax, %0"		\
					 :"=m"(ret) 			\
					 :"i"(num),"m"(arg1),"m"(arg2), "m"(arg3), "m"(arg4), "m"(arg5), "m"(arg6)	\
					 :SYSCALL_CLOBBERS, "rdi", "rsi", "rdx", "r10", "r8", "r9");				\
											\
		SYSCALL_ERROR_PROCESS(ret)			\
											\
		ret;								\
	})

#endif