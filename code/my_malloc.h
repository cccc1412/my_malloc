#ifndef MY_MALLOC_H_
#define MY_MALLOC_H_

#include <unistd.h>    
#include <sys/mman.h>
#include <stdbool.h>
#include <pthread.h>

#define ALIGN_UP_N_BYTES(size,n)  ((((size)+(n)-1)/(n))*(n))
#define ALIGN_UP_8_BYTES(size) ALIGN_UP_N_BYTES(size, 8)
#define ALIGN_UP_4KB(size) ALIGN_UP_N_BYTES(size, 4096)

#define THRESHOLD_FOR_MMAP (256 * 1024)

#define HEAP_SIZE (2 * 1024 * 1024)
#define ALIGN_2MB_SEGMENT_ATMOST_SIZE (4 * 1024 * 1024 - 4 * 1024)

#define MEM_CHUNK_MINIMUM 32

#define NBINS 128
#define CHUNK_STEP 8
#define SMALL_BIN_MAX MEM_CHUNK_MINIMUM+(NBINS-1)*CHUNK_STEP


#define GetBinArrayIndex(size) (size-MEM_CHUNK_MINIMUM)/CHUNK_STEP


// #define arena_get(ptr, size) do { \
// arena_lookup(ptr); \ 
// arena_lock(ptr, size); \
// } while(0) 

// #define arena_lookup(ptr) ptr = arena_key

// #define arena_lock(ptr, size) do { \
// if(ptr) \  
// pthread_mutex_lock(&ptr->mutex); \
// else \
// ptr = arena_get2(ptr, (size)); \
// } while(0)

struct Heap {//每次mmap出来的就是一个堆
    struct Arena *arena;//指向该堆所属的分配区
	struct Heap *prev_heap;//一个arena用到多个堆，这些堆通过双向链表连接起来
	struct Heap *next_heap;
    //size_t size;//堆大小，page对齐
};

struct Chunk {
	unsigned long ulChunkSize : 61;
	unsigned long CurBlkInUseBit : 1;
	unsigned long PrevBlkInUseBit : 1;
	unsigned long FromMmapBit : 1;
	
    struct Chunk *fd;
	struct Chunk *bk;
};

struct Arena {//每个分配区由多个Heap组成,，一个分配区可以供多个线程使用。 线程：分配区 = n:1。分配区的指针保存在tls中
	pthread_mutex_t mutex;
	struct Chunk* top;
	//struct Chunk* unsorted_bin;//bins的缓冲区，malloc时候，先从unsorted_bin中查找
	struct Chunk* bins[NBINS];
	struct Chunk* large_bin;
    //unsigned int binmap[BINMAPSIZE];//对应的bin中是否包含空闲chunk, 4*32个bit表示128个bin
    struct Arena *next;//下一个分配区，这是一个全局链表
};


struct Arena* CreateArena();
struct Heap* CreateHeap();

void *MyMalloc(size_t size);
void MyFree(void *ptr);

void MallocInit();
void *MallocByArena(size_t size);
void *MallocBymmap(size_t size);
void *MallocByHeapList(size_t size);
int GetAndInsertHeap(void); 
void *GetFreeChunkFromHeap(struct Heap *pHeap, size_t size);
int IsTheLastChunkInHeap(struct Chunk *pChunk);
void FreeHeap(struct Heap *pHeap);
void DeleteChunkInBin(struct Chunk* chunk_ptr);

void print_bins();
void print_top();

#endif//MY_MALLOC_H_