#ifndef STDLIB_H
#define STDLIB_H

#include "stddef.h"

char *itoa(int value, char *str, int base);
char *ltoa(long value, char *str, int base);

void *malloc(size_t size);
void free(void *ptr);

#endif