#ifndef MMAN_H
#define MMAN_H

#include "../stddef.h"

#define MAP_FAILED (void *)-1

#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4

#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *addr, size_t length);

#endif