#include "string.h"

char *strchr(const char *s, int c)
{
	if(s == NULL)
		return NULL;
	
	const char *p = s;

	for (; *p != '\0'; p++)
	{
		if(*p == c)
			return (char *)p;
	}
	
	return NULL;
}
