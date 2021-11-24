#include "stdio.h"
#include "threads.h"

int child(void *arg)
{
	printf("in new thread\n");

	sleep(5);

	return 3;
}

int main(void)
{
	thrd_t thr;
	thrd_create(&thr, child, NULL);

	int res;
	thrd_join(thr, &res);

	printf("in main thread, result = %d\n", res);
	
	return 0;
}
