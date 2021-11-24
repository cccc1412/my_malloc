#ifndef PRCTL_H
#define PRCTL_H

#define ARCH_SET_GS	0x1001
#define ARCH_SET_FS	0x1002
#define ARCH_GET_FS	0x1003
#define ARCH_GET_GS	0x1004

int arch_prctl(int code, unsigned long addr);

#endif
