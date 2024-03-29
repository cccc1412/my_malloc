#ifndef STRING_H
#define STRING_H

#include "stddef.h"

size_t strlen(const char *s);

char *strchr(const char *s, int c);

char *strncpy(char *dest, const char *src, size_t n);

char *strerror(int errnum);

#endif
