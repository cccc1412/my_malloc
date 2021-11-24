#include "stdlib.h"
#include "stddef.h"

#define ITOA_BUFFER_LEN 24

char *itoa(int value, char *str, int base)
{
	return ltoa(value, str, base);
}

char *ltoa(long value, char *str, int base)
{
	if(str == NULL)
		return NULL;

	base = 10;

	int index_str = 0;

	unsigned long uvalue;
	if(value >= 0)
	{
		uvalue = value;
	}
	else
	{
		uvalue = (unsigned long)(-value);
		
		str[index_str++] = '-';
	}

	char buffer[ITOA_BUFFER_LEN];
	int index_buffer = 0;

	for(;;)
	{
		int mod = uvalue % base;
		buffer[index_buffer++] = "0123456789"[mod];
		
		uvalue = uvalue / base;

		if(uvalue == 0)
			break;
	}

	for(;;)
	{	
		if(--index_buffer < 0)
			break;

		str[index_str++] = buffer[index_buffer];
	}

	str[index_str] = '\0';
	
	return str;
}

