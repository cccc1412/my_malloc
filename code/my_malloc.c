#include "my_malloc.h"
#include <pthread.h>

#define GetHeapForChunk(ptr) (struct Heap*)((unsigned long)(ptr)&~(HEAP_SIZE-1))//后面用


static bool malloc_initialized = false;

static struct Heap *g_pHeapList = NULL;
static struct Arena *g_pArenaList = NULL;
__thread struct Arena* arena_key = NULL; //tls，指向该线程的分配区
pthread_mutex_t arena_list_mutex;

struct Arena* CreateArena() {
    struct Arena* a;
	struct Heap* h;
	h = CreateHeap();
	a = h->arena = (struct Arena*)(h+1);//加一个struct heap大小
	struct Chunk* top_chunk = (struct Chunk*)(a + 1);//加一个chunk头部大小，就是topchunk地址
	a->top = top_chunk;
	top_chunk->ulChunkSize = HEAP_SIZE - sizeof(struct Arena) - sizeof(struct Heap);
	top_chunk->CurBlkInUseBit = 0;
	top_chunk->PrevBlkInUseBit = 1;
	top_chunk->FromMmapBit = 0;
	top_chunk->fd = NULL;
	top_chunk->bk = NULL;
	*((unsigned long*)((char*)top_chunk + top_chunk->ulChunkSize) - 1) = top_chunk->ulChunkSize;
	pthread_mutex_init(&a->mutex, NULL);
	return a;
}

struct Heap* CreateHeap() {
	char *addr = (char *)mmap(NULL, ALIGN_2MB_SEGMENT_ATMOST_SIZE, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(addr == MAP_FAILED)
		return -1;
	
	char *heap_base = (char *)ALIGN_UP_N_BYTES((unsigned long)addr, HEAP_SIZE);
	
	if(addr == heap_base)
		munmap((void*)(addr + HEAP_SIZE), ALIGN_2MB_SEGMENT_ATMOST_SIZE - HEAP_SIZE);
	else if(addr + ALIGN_2MB_SEGMENT_ATMOST_SIZE == heap_base + HEAP_SIZE)
		munmap((void*)addr, ALIGN_2MB_SEGMENT_ATMOST_SIZE - HEAP_SIZE);
	else
	{
		munmap((void*)addr, heap_base - addr);
		munmap((void*)(heap_base + HEAP_SIZE), ALIGN_2MB_SEGMENT_ATMOST_SIZE - HEAP_SIZE - (heap_base - addr));
	}



	struct Heap *pHeap = (struct Heap *)heap_base;
	
	pHeap->prev_heap = g_pHeapList;
	pHeap->next_heap = NULL;
	pHeap->arena = arena_key;

	if(g_pHeapList != NULL)
		g_pHeapList->next_heap = pHeap;	
	
	g_pHeapList = pHeap;
	return heap_base;
}



void MallocInit() {////主线程才需要初始化
    if(malloc_initialized) return;
    struct Arena main_arena;
	arena_key = CreateArena();
    g_pArenaList = &main_arena;
    g_pArenaList->next = g_pArenaList;
    malloc_initialized = 1;
}

struct Arena* SearchFromArenaList() {//找到一个没有加锁的空闲分配区
	struct Arena* arena_ptr = g_pArenaList;
	for(;arena_ptr!=NULL;arena_ptr = arena_ptr->next) {
		if(0 == pthread_mutex_trylock(&arena_ptr->mutex)) {
			return arena_ptr;
		}
	}
	return NULL;
}

void *MyMalloc(size_t size) {
	//MallocInit();
	if(size <= 0) return NULL;
	if(arena_key != NULL) 
		pthread_mutex_lock(&arena_key->mutex);
	pthread_mutex_lock(&arena_list_mutex);
	while(NULL == arena_key) {
		struct Arena* addr = SearchFromArenaList();
		if(NULL == addr) {
			addr = CreateArena();
			//arena_key = addr;//新arena加入tls
			addr->next = g_pArenaList;//新arena加入链表
			g_pArenaList = addr;
		} 
		arena_key = addr;
	};
	pthread_mutex_unlock(&arena_list_mutex);
	//转化成实际size
	size = ALIGN_UP_8_BYTES(size + sizeof(struct Chunk) - 16 + 8);
	printf("chunk size :%d ",size);
	void *p = MallocByArena(size);
	pthread_mutex_unlock(&arena_key->mutex);
	return p;
}


void *GetChunkFromBins(size_t size) {//找到满足要求的chunk地址
	//struct Chunk* ubin_ptr = arena_key->unsorted_bin;
	//先从unsorted bin中找
	// while(ubin_ptr != NULL) {
	// 	if(ubin_ptr->ulChunkSize == size) {
	// 		return ubin_ptr;
	// 	}
	// 	int index = (ubin_ptr->ulChunkSize - MEM_CHUNK_MINIMUM) / CHUNK_STEP;
	// 	if(index >= NBINS) {
	// 		index = NBINS - 1;
	// 	}
	// 	struct Chunk* next_ubin_ptr = ubin_ptr->fd;

	// 	ubin_ptr->fd = arena_key->bins[index]->fd;
	// 	arena_key->bins[index]->bk = ubin_ptr;
	// 	ubin_ptr->bk = arena_key->bins[index];
	// 	ubin_ptr = next_ubin_ptr;
	// }
	//先根据size找到从哪个bins数组里找
	int index = (size - MEM_CHUNK_MINIMUM) / CHUNK_STEP;
	if(index >= NBINS) {//从largebin里找
		struct Chunk* chunk_ptr = arena_key->large_bin;
		while(chunk_ptr != NULL) {
			if(chunk_ptr->ulChunkSize >= size){
				return chunk_ptr;
			}
			if (chunk_ptr == chunk_ptr->fd) chunk_ptr->fd = NULL;
			if (chunk_ptr == chunk_ptr->bk) chunk_ptr->bk = NULL;
			chunk_ptr = chunk_ptr->fd;
		}
		return NULL;
	}
	for (int i = index; i < NBINS; i++) {
		if(arena_key->bins[i]!=NULL) {
			struct Chunk* addr = arena_key->bins[i];
			if(arena_key->bins[i]->fd != NULL) {
				arena_key->bins[i]->fd->bk = NULL;
			}
			arena_key->bins[i] = arena_key->bins[i]->fd;
			return addr;
		}
	}
	return NULL;
}

void *MallocFromBins(void *addr, size_t size) {//chunk地址，size
	struct Chunk* chunk_ptr = (struct Chunk*)addr;
	struct Chunk* remain_chunk_ptr = (struct Chunk*)((char*)chunk_ptr + size);
	size_t ori_size = chunk_ptr->ulChunkSize;
	DeleteChunkInBin(chunk_ptr);
	if( ori_size  >= MEM_CHUNK_MINIMUM + size) {
		remain_chunk_ptr->CurBlkInUseBit = 0;
		remain_chunk_ptr->FromMmapBit = 0;
		remain_chunk_ptr->PrevBlkInUseBit = 1;
		remain_chunk_ptr->ulChunkSize = ori_size - size;
		remain_chunk_ptr->fd = NULL;
		remain_chunk_ptr->bk = NULL;
		*((unsigned long*)((char*)remain_chunk_ptr + remain_chunk_ptr->ulChunkSize) - 1) =  remain_chunk_ptr->ulChunkSize;
		*((unsigned long*)remain_chunk_ptr - 1) = size;
		AddChunkInBin(remain_chunk_ptr);
		chunk_ptr->ulChunkSize = size;
		chunk_ptr->CurBlkInUseBit = 1;
	}else {//剩余部分不够再分配一次的最小空间，则全部分配出去
		chunk_ptr->ulChunkSize = ori_size;
		chunk_ptr->CurBlkInUseBit = 1;
		chunk_ptr->fd = NULL;
		chunk_ptr->bk = NULL;
		if(chunk_ptr->PrevBlkInUseBit == 0)
			*((unsigned long*)((char*)chunk_ptr + chunk_ptr->ulChunkSize) - 1) =  chunk_ptr->ulChunkSize;
	}
	return addr + sizeof(struct Chunk) -16;
}

bool GetTopChunk(size_t size) {
	struct Chunk* top = arena_key->top;
	if(top->ulChunkSize >= size + sizeof(struct Chunk)) {//topchunk分配出来至少还应该留个chunk头部
		return true;
	}
	return false;
}

void *MallocFromTopChunk(size_t size) {
	struct Chunk *top_chunk = arena_key->top;
	struct Chunk *pNextChunk = (struct Chunk *)((char *)top_chunk + size);
	pNextChunk->CurBlkInUseBit = 0;
	pNextChunk->PrevBlkInUseBit = 1;
	pNextChunk->FromMmapBit = 0;
	pNextChunk->ulChunkSize = top_chunk->ulChunkSize - size;
	pNextChunk->fd = NULL;
	pNextChunk->bk = NULL;
	
	top_chunk->ulChunkSize = size;
	top_chunk->CurBlkInUseBit = 1;
	top_chunk->fd = NULL;
	top_chunk->bk = NULL;

	arena_key->top = pNextChunk;
	*((unsigned long *)((char *)pNextChunk + pNextChunk->ulChunkSize) - 1) = pNextChunk->ulChunkSize;
	*((unsigned long *)((char*)top_chunk + top_chunk->ulChunkSize) - 1) = top_chunk->ulChunkSize;

	// 	pChunk->ulChunkSize = size;
	// }

	//pChunk->CurBlkInUseBit = 1;
	void* addr = (char *)top_chunk + sizeof(struct Chunk) -16;
	return addr;
}

void ExpendTopChunk(size_t size) {
	struct Heap *new_heap = CreateHeap();
	struct Chunk* top_chunk = (struct Chunk*)((char*)new_heap + sizeof(struct Heap));
	top_chunk->CurBlkInUseBit = 0;
	top_chunk->PrevBlkInUseBit = 1;
	top_chunk->FromMmapBit = 0;
	top_chunk->ulChunkSize = HEAP_SIZE - sizeof(struct Heap);
	*((unsigned long *)((char *)top_chunk + top_chunk->ulChunkSize) - 1) = top_chunk->ulChunkSize;//下一块的第一个字节放的是上一块的大小
	//arena_key->top 
	AddChunkInBin(arena_key->top);
	arena_key->top = top_chunk;
}

void *	MallocByArena(size_t size) {
	void *addr;
	if(size >= THRESHOLD_FOR_MMAP) {//如果大于阈值，直接用mmap
		addr = MallocBymmap(size);
		printf("malloc from mmap!\n");
		return addr;
	} else if (NULL != (addr = GetChunkFromBins(size))) {//先从Bins里找
		addr =  MallocFromBins(addr, size);
		printf("malloc from bins!\n");
		return addr;
	} else if(GetTopChunk(size)) {//不行再从TopChunk里找
		addr = MallocFromTopChunk(size);
		printf("malloc from top chunk!\n");
		return addr;
	} else { //使用sbrk或mmap增加topchunk后再分配
		ExpendTopChunk(size);
		addr = MallocFromTopChunk(size);
		printf("topchunk unsatisified, new heap and malloc from top chunk!\n");
		return addr;
	}
}

void *MallocBymmap(size_t size) {
	size = ALIGN_UP_4KB(size);
	void *addr = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(addr == MAP_FAILED)
		return NULL;

	struct Chunk *p = (struct Chunk *)addr;

	p->CurBlkInUseBit = 1;
	p->PrevBlkInUseBit = 1;
	p->FromMmapBit = 1;
	p->ulChunkSize = size;

	return (char *)addr + sizeof(struct Chunk) - 16;
}

void AddChunkInBin(struct Chunk* chunk_ptr) {
	size_t size = chunk_ptr->ulChunkSize;
	int index = (size - MEM_CHUNK_MINIMUM) / CHUNK_STEP;
	if(index >= NBINS) {//largebin
		struct Chunk* cur_chunk_ptr = arena_key->large_bin;
		if(cur_chunk_ptr == NULL) {
			// chunk_ptr->fd = NULL;
			// chunk_ptr->bk = NULL;
			arena_key->large_bin = chunk_ptr;
			return;
		}else {
			while(cur_chunk_ptr->ulChunkSize < chunk_ptr->ulChunkSize) {
				if(cur_chunk_ptr->fd != NULL)
					cur_chunk_ptr = cur_chunk_ptr->fd;
				else {
					cur_chunk_ptr->fd = chunk_ptr;
					chunk_ptr->bk = cur_chunk_ptr;
					return;
				}
			}
			chunk_ptr->fd = cur_chunk_ptr;
			chunk_ptr->bk = cur_chunk_ptr->bk;
			if(cur_chunk_ptr->bk != NULL)
				cur_chunk_ptr->bk->fd = chunk_ptr;
			cur_chunk_ptr->bk = chunk_ptr;
			if(arena_key->large_bin == cur_chunk_ptr) {
				arena_key->large_bin = chunk_ptr;
			}
			return;
		}
		// if(arena_key->large_bin != NULL) {
		// 	arena_key->large_bin->bk = chunk_ptr;
		// }
		// chunk_ptr->fd = arena_key->large_bin;
		// arena_key->large_bin = chunk_ptr;
		// return;
	}

	chunk_ptr->fd = arena_key->bins[index];
	chunk_ptr->bk = NULL;
	if(arena_key->bins[index] != NULL) {
		arena_key->bins[index]->bk = chunk_ptr;
	}
	arena_key->bins[index] = chunk_ptr;
}

void DeleteChunkInBin(struct Chunk* chunk_ptr) {
	int index = (chunk_ptr->ulChunkSize - MEM_CHUNK_MINIMUM) / CHUNK_STEP;
	if(index < NBINS && index >=0 &&arena_key->bins[index] != NULL) {//从smallbin中删除
		// if(chunk_ptr->fd != NULL) {
		// 	arena_key->bins[index] = chunk_ptr->fd;
		// 	chunk_ptr->fd->bk = NULL;
		// 	chunk_ptr->fd = chunk_ptr->bk = NULL;
		// 	return;
		// }
		// arena_key->bins[index] = NULL;
		if(chunk_ptr->fd != NULL) {
			chunk_ptr->fd->bk = chunk_ptr->bk;
		}
		if(chunk_ptr->bk != NULL) {
			chunk_ptr->bk->fd = chunk_ptr->fd;
		}else {//说明是第一个
			arena_key->bins[index] = chunk_ptr->fd;
		}
		chunk_ptr->fd = chunk_ptr->bk = NULL;
		return;
	} else {//largebin
		if(chunk_ptr->fd != NULL) {
			chunk_ptr->fd->bk = chunk_ptr->bk;
		}
		if(chunk_ptr->bk != NULL) {
			chunk_ptr->bk->fd = chunk_ptr->fd;
		}else{
			arena_key->large_bin = chunk_ptr->fd;
		}
		chunk_ptr->fd = chunk_ptr->bk = NULL;
		return;
	}
}

bool IsLastBlock(struct Chunk *chunk_ptr){
	int i = ((unsigned long)chunk_ptr + chunk_ptr->ulChunkSize) % HEAP_SIZE;
	if(i == 0)
		return 1;
	else
		return 0;
}

void MyFree(void *ptr) {
	if(ptr == NULL) {
		return;
	}
	struct Chunk* chunk_ptr = (struct Chunk*)((char*)ptr - sizeof(struct Chunk) + 16);
	// struct Heap* heap = GetHeapForChunk(chunk_ptr);
	// struct Arena *arena = heap->arena;
	// pthread_mutex_lock(&arena->mutex);
	if(chunk_ptr->FromMmapBit == 1) {
		munmap((void*)chunk_ptr, chunk_ptr->ulChunkSize);
		return;
	}
	pthread_mutex_lock(&arena_key->mutex);////没写完，应该对chunk所属的分配区加锁，不是线程私有变量中的分配区，因为一个线程可以释放另一个线程malloc出来的内存块
	struct Chunk* new_chunk_ptr = chunk_ptr;
	unsigned long new_size = chunk_ptr->ulChunkSize;

	if(chunk_ptr->PrevBlkInUseBit == 0) {//和前一个块合并
		unsigned long prev_size = *((unsigned long *)chunk_ptr - 1);
		struct Chunk* pre_chunk_ptr = (struct Chunk*)((char*)chunk_ptr-prev_size);
		//remove
		DeleteChunkInBin(pre_chunk_ptr);
		new_size += prev_size;
		new_chunk_ptr = pre_chunk_ptr;
	}

	if(chunk_ptr != arena_key->top && !IsLastBlock(chunk_ptr)) {
		struct Chunk* next_chunk_ptr = (struct Chunk*)((char*)chunk_ptr + chunk_ptr->ulChunkSize);
		if(next_chunk_ptr->CurBlkInUseBit == 0 && next_chunk_ptr->ulChunkSize != 0) {//和后一个块合并
			//remove
			if(next_chunk_ptr != arena_key->top)
				DeleteChunkInBin(next_chunk_ptr);
			new_size += next_chunk_ptr->ulChunkSize;
			if(next_chunk_ptr == arena_key->top) {//下一个chunk是top_chunk才会更新top指针
				arena_key->top = new_chunk_ptr;
			}
		}
	}
	printf("new size:%ld\n", new_size);
	if(new_size == HEAP_SIZE - sizeof(struct Heap)) { //包含分配区的那个heap不会被mumap掉
		//如果mumap的是最后一个heap需要更新top
		struct Heap* heap = (struct Heap*)((char*)new_chunk_ptr-sizeof(struct Heap));
		if(heap->next_heap == NULL) {
			struct Heap* pre_heap = heap->prev_heap;
			int last_chunk_size = *((unsigned long*)((char*)pre_heap + HEAP_SIZE) - 1);
			struct Chunk* last_chunk =(struct Chunk *)((char*)pre_heap + HEAP_SIZE - *((unsigned long*)((char*)pre_heap + HEAP_SIZE) - 1));
			if(last_chunk->CurBlkInUseBit == 0)
				arena_key->top = last_chunk;
			else
				arena_key->top = (struct Chunk*)((char*)last_chunk + last_chunk->ulChunkSize);
			g_pHeapList = heap->prev_heap;
		}

		if(heap->prev_heap != NULL) {
			heap->prev_heap->next_heap = heap->next_heap;
		}
		if(heap->next_heap != NULL) {
			heap->next_heap->prev_heap = heap->prev_heap;
		}
		//munmap((void*)((char*)new_chunk_ptr-sizeof(struct Heap)),HEAP_SIZE);
		pthread_mutex_unlock(&arena_key->mutex);
		return;
	}

	new_chunk_ptr->CurBlkInUseBit = 0;
	new_chunk_ptr->PrevBlkInUseBit = 1;
	new_chunk_ptr->FromMmapBit = 0;
	new_chunk_ptr->ulChunkSize = new_size;
	new_chunk_ptr->fd = NULL;
	new_chunk_ptr->bk = NULL;

	*((unsigned long*)((char*)new_chunk_ptr + new_chunk_ptr->ulChunkSize) - 1) = new_size;
	if(chunk_ptr != arena_key->top && !IsLastBlock(chunk_ptr)) {
		struct Chunk* next_chunk_ptr = (struct Chunk*)((char*)chunk_ptr + chunk_ptr->ulChunkSize);
		next_chunk_ptr->PrevBlkInUseBit = 0;
	}
	if(new_chunk_ptr != arena_key->top) {//非topchunk才需要加入bins
		AddChunkInBin(new_chunk_ptr);
	}
	pthread_mutex_unlock(&arena_key->mutex);
	return;
}

void print_bins() {
	printf("printing bins.......\n");
	struct Arena* arena = arena_key;
	for(int i = 0 ;i < NBINS; i++) {
		if(arena->bins[i] == NULL) continue;
		printf("printing BIN %d\n",i);
		struct Chunk* chunk_ptr = arena->bins[i];
		while(chunk_ptr != NULL){
			printf("chunk size:%ld\n",chunk_ptr->ulChunkSize);
			printf("chunk CurBlkUseBit:%d\n",chunk_ptr->CurBlkInUseBit);
			printf("chunk PrevBlkUseBit:%d\n",chunk_ptr->PrevBlkInUseBit);
			printf("chunk FromMmapBit:%d\n",chunk_ptr->FromMmapBit);
			printf("\n");
			chunk_ptr = chunk_ptr->fd;
		}
		printf("\n\n\n");
	}
}

void print_heaps() {
	
}

void print_top() {
	printf("top ptr:%x\n", arena_key->top);
}
