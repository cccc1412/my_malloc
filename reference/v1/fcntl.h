#ifndef FCNTL_H
#define FCNTL_H

#define O_RDONLY	00000000
#define O_WRONLY	00000001
#define O_RDWR		00000002

#define O_CREAT     00000100
#define O_EXCL	    00000200
#define O_TRUNC		00001000
#define O_APPEND	00002000

#define S_IRWXU 	00700
#define S_IRUSR 	00400
#define S_IWUSR 	00200
#define S_IXUSR 	00100

#define S_IRWXG 	00070
#define S_IRGRP 	00040
#define S_IWGRP 	00020
#define S_IXGRP 	00010

#define S_IRWXO 	00007
#define S_IROTH 	00004
#define S_IWOTH 	00002
#define S_IXOTH 	00001

int open(const char *pathname, int flags, ...);

typedef unsigned int mode_t;
int creat(const char *pathname, mode_t mode);

#endif
