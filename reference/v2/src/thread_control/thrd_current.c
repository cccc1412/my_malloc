#include "threads.h"

thrd_t thrd_current(void)
{
	unsigned long tcb_addr;
	
	asm volatile("movq %%fs:0, %0" 
		 		 :"=r"(tcb_addr));

	return tcb_addr;
}
