#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/mman.h"

#define LEN 10000
void _start(void)
{
	void *p_array[LEN];
	for(int i = 0;i<LEN;i++){
		void *p = malloc(8*i);
		p_array[i] = p;
	}
	for(int i = 0 ; i < LEN/2; i++) {
		free(p_array[i]);
	}
	// void *p = malloc(8);
	// free(p);
	// printf("test");
	_exit(EXIT_SUCCESS);
}
