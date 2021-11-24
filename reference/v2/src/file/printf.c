#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "stdlib.h"

#define PRINTF_BUFFER_LEN 4096

int printf(const char *format, ...)
{
	if(format == NULL)
		return 0;

	size_t len_format = strlen(format);
	if(len_format == 0)
		return 0;
	
	char buffer[PRINTF_BUFFER_LEN] = {0};

	const char *pstart_format = format;
	const char *pend_format = NULL;
	char *pstart_buffer = buffer;
	
	__builtin_va_list arg;
	__builtin_va_start(arg, format);

	for(; ;)
	{
		pend_format = strchr(pstart_format, '%');
		if(pend_format == NULL)
		{
			strncpy(pstart_buffer, pstart_format, len_format + 1 - (pstart_format - format));
			break;
		}

		int len = pend_format - pstart_format;
		strncpy(pstart_buffer, pstart_format, len);
		pstart_buffer += len;

		if(pend_format[1] == 'd')
		{
			int value = __builtin_va_arg(arg, int);

			itoa(value, pstart_buffer, 10);
			pstart_buffer += strlen(pstart_buffer);

			pstart_format = pend_format + 2;
			continue;
		}

		if(pend_format[1] == 'c')
		{
			char c = __builtin_va_arg(arg, int);
			*pstart_buffer = c;
			pstart_buffer++;

			pstart_format = pend_format + 2;
			continue;
		}

		if(pend_format[1] == 's')
		{
			char *pstr = (char *)__builtin_va_arg(arg, long);
			
			int len_pstr = strlen(pstr);
			strncpy(pstart_buffer, pstr, len_pstr);
			pstart_buffer += len_pstr;

			pstart_format = pend_format + 2;
			continue;
		}

		if((pend_format[1] == 'l') && (pend_format[2] == 'd'))
		{
			long value = __builtin_va_arg(arg, long);

			ltoa(value, pstart_buffer, 10);
			pstart_buffer += strlen(pstart_buffer);

			pstart_format = pend_format + 3;
			continue;
		}

		__builtin_va_end(arg);
		
		return -1;
	}

	__builtin_va_end(arg);

	return write(STDOUT_FILENO, buffer, strlen(buffer));
}
