#include "stdlib.h"
#include "unistd.h"

typedef void (*EXIT_CALLBACK_TYPE)(void);

#define EXIT_CALLBACK_TABLE_SIZE 32

static EXIT_CALLBACK_TYPE gExitCallbackTable[EXIT_CALLBACK_TABLE_SIZE];
static int gExitCallbackTableIter = 0;

int atexit(void (*function)(void))
{
	if(function == NULL)
		return -1;

	if(gExitCallbackTableIter >= EXIT_CALLBACK_TABLE_SIZE)
		return -1;

	gExitCallbackTable[gExitCallbackTableIter] = function;

	gExitCallbackTableIter++;
	
	return 0;
}

void exit(int status)
{
	for(int i = gExitCallbackTableIter - 1; i >= 0; i--)
		gExitCallbackTable[i]();
	
	exit_group(status);
}
