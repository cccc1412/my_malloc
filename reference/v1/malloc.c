#include "stdlib.h"
#include "sys/mman.h"

#define ALIGN_UP_N_BYTES(size, n) ((((size) + (n) - 1) / (n)) * (n))
#define ALIGN_UP_8_BYTES(size) ALIGN_UP_N_BYTES(size, 8)
#define ALIGN_UP_4KB(size) ALIGN_UP_N_BYTES(size, 4096)

#define THRESHOLD_FOR_MMAP (256 * 1024)
#define SEGMENT_SIZE (2 * 1024 * 1024)

#define MEM_BLOCK_MINIMUM 16

#define NBINS 64

struct SLMemSegment
{
	struct SLMemSegment *pPrevMemSegment;
	struct SLMemSegment *pNextMemSegment;
};

struct SLMemBlock //相当于chunk
{
	unsigned long CurBlkInUseBit : 1;
	unsigned long PrevBlkInUseBit : 1;
	unsigned long FromMmapBit : 1;
	unsigned long ulBlockSize : 61;
};

struct MallocState //每个分配区是malloc_state的一个实例
{
	mutex_t mutex;
	struct SLMemBlock* bins[NBINS];
};


struct HeapInfo 
{
	/* data */
};
static struct SLMemSegment *g_pSegmentList = NULL;

void *MallocBymmap(size_t size);
void *MallocBySegmentList(size_t size);
int GetAndInsertSegment(void);
void *GetFreeMemBlockFromSegment(struct SLMemSegment *pSegment, size_t size);
int IsTheLastBlockInSegment(struct SLMemBlock *pBlock);
void FreeSegment(struct SLMemSegment *pSegment);

void *malloc(size_t size)
{
	size = ALIGN_UP_8_BYTES(size);
	
	if(size < MEM_BLOCK_MINIMUM)
		size = MEM_BLOCK_MINIMUM;
	else
	{
		size += sizeof(struct SLMemBlock);//加上chunk头部8字节
	
		if(size >= THRESHOLD_FOR_MMAP)//如果大于阈值，直接用mmap
			return MallocBymmap(size);
	}

	return MallocBySegmentList(size);//否则从segmentList中分配
}

void *MallocBymmap(size_t size)
{
	size = ALIGN_UP_4KB(size);
	
	void *addr = mmap(NULL, size, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(addr == MAP_FAILED)
		return NULL;

	struct SLMemBlock *p = (struct SLMemBlock *)addr;

	p->CurBlkInUseBit = 1;
	p->PrevBlkInUseBit = 1;
	p->FromMmapBit = 1;
	p->ulBlockSize = size;

	return (char *)addr + sizeof(struct SLMemBlock);
}

void *MallocBySegmentList(size_t size)
{
	for(struct SLMemSegment *pSegment = g_pSegmentList; pSegment != NULL; pSegment = pSegment->pNextMemSegment)
	{
		void* addr = GetFreeMemBlockFromSegment(pSegment, size);
		if(addr != NULL)
			return addr;
	}

	if(GetAndInsertSegment() == -1)
		return NULL;

	return GetFreeMemBlockFromSegment(g_pSegmentList, size);
}

int GetAndInsertSegment(void)
{
	void *addr = mmap(NULL, SEGMENT_SIZE, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(addr == MAP_FAILED)
		return -1;

	struct SLMemSegment *pSegment = (struct SLMemSegment *)addr;
	
	pSegment->pNextMemSegment = g_pSegmentList;
	pSegment->pPrevMemSegment = NULL;

	if(g_pSegmentList != NULL)
		g_pSegmentList->pPrevMemSegment = pSegment;	
	
	g_pSegmentList = pSegment;

	struct SLMemBlock *pFirstBlock = (struct SLMemBlock *)((char *)addr + sizeof(struct SLMemSegment));

	pFirstBlock->CurBlkInUseBit = 0;
	pFirstBlock->PrevBlkInUseBit = 1;
	pFirstBlock->FromMmapBit = 0;
	pFirstBlock->ulBlockSize = SEGMENT_SIZE - sizeof(struct SLMemSegment);

	*((unsigned long *)((char *)pFirstBlock + pFirstBlock->ulBlockSize) - 1) = pFirstBlock->ulBlockSize;

	return 0;
}

void *GetFreeMemBlockFromSegment(struct SLMemSegment *pSegment, size_t size)
{
	struct SLMemBlock *pBlock = (struct SLMemBlock *)((char *)pSegment + sizeof(struct SLMemSegment));
	struct SLMemBlock *pEndBlock = (struct SLMemBlock *)((char *)pSegment + SEGMENT_SIZE);
	
	for(; pBlock < pEndBlock; pBlock = (struct SLMemBlock *)((char *)pBlock + pBlock->ulBlockSize))
	{
		if(pBlock->CurBlkInUseBit == 1)
			continue;

		if(pBlock->ulBlockSize < size)
			continue;

		if(pBlock->ulBlockSize - size < MEM_BLOCK_MINIMUM)
		{
			struct SLMemBlock *pNextBlock = (struct SLMemBlock *)((char *)pBlock + pBlock->ulBlockSize);

			if(pNextBlock < pEndBlock)
				pNextBlock->PrevBlkInUseBit = 1;
		}
		else
		{
			struct SLMemBlock *pNextBlock = (struct SLMemBlock *)((char *)pBlock + size);

			pNextBlock->CurBlkInUseBit = 0;
			pNextBlock->PrevBlkInUseBit = 1;
			pNextBlock->FromMmapBit = 0;
			pNextBlock->ulBlockSize = pBlock->ulBlockSize - size;
			
			*((unsigned long *)((char *)pNextBlock + pNextBlock->ulBlockSize) - 1) = pNextBlock->ulBlockSize;

			pBlock->ulBlockSize = size;
		}

		pBlock->CurBlkInUseBit = 1;
		return (char *)pBlock + sizeof(struct SLMemBlock);
	}

	return NULL;
}

void free(void *ptr)
{
	if(ptr == NULL)
		return;
	
	struct SLMemBlock *pBlock = (struct SLMemBlock *)((char *)ptr - sizeof(struct SLMemBlock));
	if(pBlock->FromMmapBit == 1)
	{
		munmap((void*)pBlock, pBlock->ulBlockSize);
		return;
	}

	size_t len = pBlock->ulBlockSize;

	int bLast = IsTheLastBlockInSegment(pBlock);

	if(!bLast)
	{
		struct SLMemBlock *pNextBlock = (struct SLMemBlock *)((char *)pBlock + len);
		if(pNextBlock->CurBlkInUseBit == 0)
			len += pNextBlock->ulBlockSize;
	}

	if(pBlock->PrevBlkInUseBit == 0)
	{
		unsigned long prev_size = *((unsigned long *)pBlock - 1);
		len += prev_size;

		pBlock = (struct SLMemBlock *)((char *)pBlock - prev_size);
	}

	pBlock->ulBlockSize = len;
	pBlock->CurBlkInUseBit = 0;
	pBlock->PrevBlkInUseBit = 1;
	pBlock->FromMmapBit = 0;

	bLast = IsTheLastBlockInSegment(pBlock);

	struct SLMemBlock *pNextBlock = (struct SLMemBlock *)((char *)pBlock + pBlock->ulBlockSize);
	*((unsigned long *)pNextBlock - 1) = pBlock->ulBlockSize;

	if(!bLast)
	{
		pNextBlock->PrevBlkInUseBit = 0;
	}
	else
	{
		if(pBlock->ulBlockSize == SEGMENT_SIZE - sizeof(struct SLMemSegment))
			FreeSegment((struct SLMemSegment *)((char *)pBlock - sizeof(struct SLMemSegment)));
	}
}

int IsTheLastBlockInSegment(struct SLMemBlock *pBlock)
{
	char *pEnd = (char *)pBlock + pBlock->ulBlockSize;
	
	for(struct SLMemSegment *pSegment = g_pSegmentList; pSegment != NULL; pSegment = pSegment->pNextMemSegment)
	{
		char *pSegmentEnd = (char *)pSegment + SEGMENT_SIZE;

		if(((char *)pBlock > (char *)pSegment) && (pEnd < pSegmentEnd))
			return 0;
		
		if(pEnd == pSegmentEnd)
			return 1;
	}

	return 0;
}

void FreeSegment(struct SLMemSegment *pSegment)
{
	if(g_pSegmentList == pSegment)
	{
		g_pSegmentList = pSegment->pNextMemSegment;

		if(g_pSegmentList != NULL)
			g_pSegmentList->pPrevMemSegment = NULL;
	}
	else
	{
		pSegment->pPrevMemSegment->pNextMemSegment = pSegment->pNextMemSegment;

		if(pSegment->pNextMemSegment != NULL)
			pSegment->pNextMemSegment->pPrevMemSegment = pSegment->pPrevMemSegment;
	}

	munmap((void*)pSegment, SEGMENT_SIZE);
}
