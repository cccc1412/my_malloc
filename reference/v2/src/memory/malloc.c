#include "stdlib.h"
#include "sys/mman.h"

#define ALGIN_UP_N_BYTES(size, n) ((((size) + (n) - 1) / (n)) * (n))
#define ALGIN_UP_8_BYTES(size) ALGIN_UP_N_BYTES(size, 8)
#define ALGINI_UP_4KB(size) ALGIN_UP_N_BYTES(size, 4096)
#define ALGINI_UP_1KB(size) ALGIN_UP_N_BYTES(size, 1024)

#define SMALL_MEM_BLOCK_MINIMUM 32
#define SMALL_MEM_BLOCK_MAXIMUM 1016
#define SMALL_MEM_BLOCK_STEP 8

#define SMALL_MEM_BLOCK_LIST_NUM ((SMALL_MEM_BLOCK_MAXIMUM - SMALL_MEM_BLOCK_MINIMUM) / SMALL_MEM_BLOCK_STEP + 1)

#define LARGE_MEM_BLOCK_MINIMUM 1024
#define LARGE_MEM_BLOCK_MAXIMUM (255 * 1024)
#define LARGE_MEM_BLOCK_STEP 1024

#define LARGE_MEM_BLOCK_LIST_NUM ((LARGE_MEM_BLOCK_MAXIMUM - LARGE_MEM_BLOCK_MINIMUM) / LARGE_MEM_BLOCK_STEP + 1)

#define EXTRA_LARGE_MEM_BLOCK_MINIMUM (256 * 1024)

#define SEGMENT_SIZE (2 * 1024 * 1024)
#define ALIGN_2MB_SEGMENT_ATMOST_SIZE (4 * 1024 * 1024 - 4 * 1024)

typedef struct _SLMemBlock
{
	unsigned long CurBlkInUseBit : 1;
	unsigned long PrevBlkInUseBit : 1;
	unsigned long FromMmapBit : 1;
	unsigned long ulBlockSize : 61;
}SLMemBlock;

typedef struct _SLSmallMemBlock
{
	SLMemBlock MemBlockHead;
	
	struct _SLSmallMemBlock *pNext;
	struct _SLSmallMemBlock *pPrev;
}SLSmallMemBlock;

typedef struct _SLLargeMemBlock
{
	SLMemBlock MemBlockHead;

	struct _SLLargeMemBlock *pNext;
	struct _SLLargeMemBlock *pPrev;

	struct _SLLargeMemBlock *pLeftChild;
	struct _SLLargeMemBlock *pRightChild;
	struct _SLLargeMemBlock *pParent;
}SLLargeMemBlock;

typedef struct _SLSmallMemBlock SLBackupNode;

typedef struct _SLHeapManager
{	
	SLSmallMemBlock *SmallMemBlockListArray[SMALL_MEM_BLOCK_LIST_NUM];

	SLLargeMemBlock *LargeMemBlockTreeArray[LARGE_MEM_BLOCK_LIST_NUM];

	SLBackupNode *pBackupList;
}SLHeapManager;

static SLHeapManager gHeapManager;

void *MallocByExtraLargeMemBlock(size_t size);

void *MallocBySmallMemBlock(size_t size);
void *MallocBySmallMemBlockList(int index, size_t size);
void InsertSmallMemBlock(SLSmallMemBlock *pBlock);
int GetSmallMemBlockListArrayIndex(size_t size);
SLSmallMemBlock *GetFirstSmallMemBlockInList(int index);
void InsertFirstSmallMemBlockInList(int index, SLSmallMemBlock *pBlock);

void *MallocByLargeMemBlock(size_t size);
void *MallocByLargeMemBlockTree(int index, size_t size);
void InsertLargeMemBlock(SLLargeMemBlock *pBlock);
void InsertLargeMemBlockInTree(SLLargeMemBlock *pRoot, SLLargeMemBlock *pBlock);
void SearchGreaterAndCloserNodeInLargeMemBlockTree(SLLargeMemBlock *pRoot, size_t size, SLLargeMemBlock **ppAllocatedNode);
void DeleteNodeInLargeMemBlockTree(int index, SLLargeMemBlock *pNode);
void UpdateParentNodeInLargeMemBlockTree(int index, SLLargeMemBlock *pOldChild, SLLargeMemBlock *pNewChild);
int GetLargeMemBlockTreeArrayIndex(size_t size);
SLLargeMemBlock *GetLargeMemBlockInTreeNodeList(SLLargeMemBlock *pBlock);
void InsertLargeMemBlockInTreeNodeList(SLLargeMemBlock *pPrevBlock, SLLargeMemBlock *pBlock);

void *MallocByBackupList(size_t size);
int Allocate2MBAlignSegment(void);
SLSmallMemBlock *GetFirstNodeInBackupList(void);
void InsertFirstNodeInBackupList(SLBackupNode *pNode);

SLMemBlock *SplitMemBlock(SLMemBlock *pBlock, size_t size);
int IsTheLastBlockInSegment(SLMemBlock *pBlock);
void InsertMemBlockIntoDataStructure(SLMemBlock *pBlock);

void RemoveMemBlockFromDataStructure(SLMemBlock *pBlock);
void RemoveMemBlockFromSmallMemBlockList(SLSmallMemBlock *pBlock);
void RemoveMemBlockFromLargeMemBlockTree(SLLargeMemBlock *pBlock);
void RemoveMemBlockFromBackupList(SLBackupNode *pBlock);

void *malloc(size_t size)
{	
	size = ALGIN_UP_8_BYTES(size);

	if(size < SMALL_MEM_BLOCK_MINIMUM)
		size = SMALL_MEM_BLOCK_MINIMUM;
	else
		size += sizeof(SLMemBlock);

	if(size >= EXTRA_LARGE_MEM_BLOCK_MINIMUM)
		return MallocByExtraLargeMemBlock(size);//mmap

	void *addr;
	
	if(size <= SMALL_MEM_BLOCK_MAXIMUM)
	{
		addr = MallocBySmallMemBlock(size);//bins
		if(addr != NULL)
			return addr;
	}

	addr = MallocByLargeMemBlock(size);//tree
	if(addr != NULL)
		return addr;

	return MallocByBackupList(size);
}

int Allocate2MBAlignSegment(void)
{
	char *addr = (char *)mmap(NULL, ALIGN_2MB_SEGMENT_ATMOST_SIZE, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(addr == MAP_FAILED)
		return -1;
	
	char *segment_base = (char *)ALGIN_UP_N_BYTES((unsigned long)addr, SEGMENT_SIZE);
	
	if(addr == segment_base)
		munmap((void*)(addr + SEGMENT_SIZE), ALIGN_2MB_SEGMENT_ATMOST_SIZE - SEGMENT_SIZE);
	else if(addr + ALIGN_2MB_SEGMENT_ATMOST_SIZE == segment_base + SEGMENT_SIZE)
		munmap((void*)addr, ALIGN_2MB_SEGMENT_ATMOST_SIZE - SEGMENT_SIZE);
	else
	{
		munmap((void*)addr, segment_base - addr);
		munmap((void*)(segment_base + SEGMENT_SIZE), ALIGN_2MB_SEGMENT_ATMOST_SIZE - SEGMENT_SIZE - (segment_base - addr));
	}

	SLBackupNode *segment = (SLBackupNode *)segment_base;
	
	segment->MemBlockHead.CurBlkInUseBit = 0;
	segment->MemBlockHead.PrevBlkInUseBit = 1;
	segment->MemBlockHead.FromMmapBit = 0;
	segment->MemBlockHead.ulBlockSize = SEGMENT_SIZE;

	segment->pNext = NULL;
	segment->pPrev = NULL;

	*((unsigned long *)(segment_base + SEGMENT_SIZE) - 1) = SEGMENT_SIZE;

	gHeapManager.pBackupList = segment;

	return 0;
}

void *MallocByBackupList(size_t size)
{
	if(gHeapManager.pBackupList == NULL)
	{		
		if(Allocate2MBAlignSegment() == -1)
			return NULL;
	}

	SLBackupNode *pAllocatedMemBlock = GetFirstNodeInBackupList();

	SLMemBlock *pLeftBlock = SplitMemBlock((SLMemBlock *)pAllocatedMemBlock, size);

	if(pLeftBlock != NULL)
	{
		InsertMemBlockIntoDataStructure(pLeftBlock);
	}

	return (char *)pAllocatedMemBlock + sizeof(SLMemBlock);
}

void InsertMemBlockIntoDataStructure(SLMemBlock *pBlock)
{
	if(pBlock->ulBlockSize < LARGE_MEM_BLOCK_MINIMUM)
	{
		InsertSmallMemBlock((SLSmallMemBlock *)pBlock);
	}
	else if(pBlock->ulBlockSize < EXTRA_LARGE_MEM_BLOCK_MINIMUM)
	{
		InsertLargeMemBlock((SLLargeMemBlock *)pBlock);
	}
	else
	{
		InsertFirstNodeInBackupList((SLBackupNode *)pBlock);
	}	
}

void *MallocByLargeMemBlock(size_t size)
{
	int index = GetLargeMemBlockTreeArrayIndex(size);

	for(; index < LARGE_MEM_BLOCK_LIST_NUM; index++)
	{
		if(gHeapManager.LargeMemBlockTreeArray[index] == NULL)
			continue;

		void *addr = MallocByLargeMemBlockTree(index, size);
		if(addr != NULL)
			return addr;
	}

	return NULL;
}

void *MallocByLargeMemBlockTree(int index, size_t size)
{
	SLLargeMemBlock *pRoot = gHeapManager.LargeMemBlockTreeArray[index];
	SLLargeMemBlock *pAllocatedNode = NULL;

	SearchGreaterAndCloserNodeInLargeMemBlockTree(pRoot, size, &pAllocatedNode);

	if(pAllocatedNode == NULL)
		return NULL;

	SLLargeMemBlock *pNext = GetLargeMemBlockInTreeNodeList(pAllocatedNode->pNext);
	
	if(pNext != NULL)
		pAllocatedNode = pNext;
	else
		DeleteNodeInLargeMemBlockTree(index, pAllocatedNode);

	SLMemBlock *pLeftBlock = SplitMemBlock((SLMemBlock *)pAllocatedNode, size);

	if(pLeftBlock != NULL)
		InsertMemBlockIntoDataStructure(pLeftBlock);

	return (char *)pAllocatedNode + sizeof(SLMemBlock);
}

void InsertLargeMemBlockInTree(SLLargeMemBlock *pRoot, SLLargeMemBlock *pBlock)
{
	if(pBlock->MemBlockHead.ulBlockSize == pRoot->MemBlockHead.ulBlockSize)
	{
		InsertLargeMemBlockInTreeNodeList(pRoot, pBlock);
	}
	else if(pBlock->MemBlockHead.ulBlockSize < pRoot->MemBlockHead.ulBlockSize)
	{
		if(pRoot->pLeftChild == NULL)
		{
			pRoot->pLeftChild = pBlock;
			pBlock->pParent = pRoot;
		}
		else
			InsertLargeMemBlockInTree(pRoot->pLeftChild, pBlock);
	}
	else
	{
		if(pRoot->pRightChild == NULL)
		{
			pRoot->pRightChild = pBlock;
			pBlock->pParent = pRoot;
		}
		else
			InsertLargeMemBlockInTree(pRoot->pRightChild, pBlock);
	}
}

void InsertLargeMemBlock(SLLargeMemBlock *pBlock)
{
	int index = GetLargeMemBlockTreeArrayIndex(pBlock->MemBlockHead.ulBlockSize);

	pBlock->pLeftChild = NULL;
	pBlock->pRightChild = NULL;
	pBlock->pParent = NULL;
	pBlock->pNext = NULL;
	pBlock->pPrev = NULL;

	SLLargeMemBlock *pRoot = gHeapManager.LargeMemBlockTreeArray[index];
	if(pRoot == NULL)
	{
		gHeapManager.LargeMemBlockTreeArray[index] = pBlock;
	}
	else
		InsertLargeMemBlockInTree(pRoot, pBlock);
}

SLMemBlock *SplitMemBlock(SLMemBlock *pBlock, size_t size)
{
	unsigned long left_size = pBlock->ulBlockSize - size;

	pBlock->CurBlkInUseBit = 1;

	if(left_size < SMALL_MEM_BLOCK_MINIMUM)
	{		
		if(!IsTheLastBlockInSegment(pBlock))
		{
			SLMemBlock *pNextBlock = (SLMemBlock *)((char *)pBlock + pBlock->ulBlockSize);
			pNextBlock->PrevBlkInUseBit = 1;
		}

		return NULL;
	}
	else
	{
		SLMemBlock *pNextBlock = (SLMemBlock *)((char *)pBlock + size);

		pNextBlock->CurBlkInUseBit = 0;
		pNextBlock->PrevBlkInUseBit = 1;
		pNextBlock->FromMmapBit = 0;
		pNextBlock->ulBlockSize = left_size;

		*((unsigned long *)((char *)pNextBlock + left_size) - 1) = left_size;

		pBlock->ulBlockSize = size;

		return pNextBlock;
	}
}

void InsertSmallMemBlock(SLSmallMemBlock *pBlock)
{
	int index = GetSmallMemBlockListArrayIndex(pBlock->MemBlockHead.ulBlockSize);

	InsertFirstSmallMemBlockInList(index, pBlock);
}

void DeleteNodeInLargeMemBlockTree(int index, SLLargeMemBlock *pNode)
{
	SLLargeMemBlock *pMovedNode = NULL;
	SearchGreaterAndCloserNodeInLargeMemBlockTree(pNode->pRightChild, pNode->MemBlockHead.ulBlockSize, &pMovedNode);

	if(pMovedNode == NULL)
	{
		if(pNode->pLeftChild)
			pNode->pLeftChild->pParent = pNode->pParent;

		UpdateParentNodeInLargeMemBlockTree(index, pNode, pNode->pLeftChild);
		return;
	}

	UpdateParentNodeInLargeMemBlockTree(index, pNode, pMovedNode);

	if(pNode->pLeftChild)
		pNode->pLeftChild->pParent = pMovedNode;

	pMovedNode->pLeftChild = pNode->pLeftChild;

	if(pNode->pRightChild != pMovedNode)
	{
		pMovedNode->pParent->pLeftChild = pMovedNode->pRightChild;

		if(pMovedNode->pRightChild)
			pMovedNode->pRightChild->pParent = pMovedNode->pParent;
	
		pMovedNode->pRightChild = pNode->pRightChild;
		
		pNode->pRightChild->pParent = pMovedNode;
	}

	pMovedNode->pParent = pNode->pParent;
	
}

void UpdateParentNodeInLargeMemBlockTree(int index, SLLargeMemBlock *pOldChild, SLLargeMemBlock *pNewChild)
{
	if(pOldChild->pParent != NULL)
	{
		if(pOldChild->pParent->pLeftChild == pOldChild)
			pOldChild->pParent->pLeftChild = pNewChild;
		else
			pOldChild->pParent->pRightChild = pNewChild;
	}
	else
		gHeapManager.LargeMemBlockTreeArray[index] = pNewChild;
}

void SearchGreaterAndCloserNodeInLargeMemBlockTree(SLLargeMemBlock *pRoot, size_t size, SLLargeMemBlock **ppAllocatedNode)
{
	if(pRoot == NULL)
		return;

	unsigned long node_size = pRoot->MemBlockHead.ulBlockSize;

	if(size > node_size)
		return SearchGreaterAndCloserNodeInLargeMemBlockTree(pRoot->pRightChild, size, ppAllocatedNode);

	*ppAllocatedNode = pRoot;
	
	if(size == node_size)
		return;

	return SearchGreaterAndCloserNodeInLargeMemBlockTree(pRoot->pLeftChild, size, ppAllocatedNode);
}

int GetLargeMemBlockTreeArrayIndex(size_t size)
{
	if(size < LARGE_MEM_BLOCK_MINIMUM)
		return 0;
	
	int index = (int)(size / LARGE_MEM_BLOCK_STEP) - 1;
	return index;
}

void *MallocByExtraLargeMemBlock(size_t size)
{
	size = ALGINI_UP_4KB(size);
	
	void *addr = mmap(NULL, size, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(addr == MAP_FAILED)
		return NULL;

	SLMemBlock *p = (SLMemBlock *)addr;

	p->CurBlkInUseBit = 1;
	p->PrevBlkInUseBit = 1;
	p->FromMmapBit = 1;
	p->ulBlockSize = size;

	return (char *)addr + sizeof(SLMemBlock);
}

void *MallocBySmallMemBlock(size_t size)
{
	int index = GetSmallMemBlockListArrayIndex(size);

	for(; index < SMALL_MEM_BLOCK_LIST_NUM; index++)
	{
		if(gHeapManager.SmallMemBlockListArray[index] == NULL)
			continue;

		return MallocBySmallMemBlockList(index, size);
	}

	return NULL;
}

int GetSmallMemBlockListArrayIndex(size_t size)
{
	return (size - SMALL_MEM_BLOCK_MINIMUM) / SMALL_MEM_BLOCK_STEP;
}

void *MallocBySmallMemBlockList(int index, size_t size)
{
	SLSmallMemBlock *pBlock = GetFirstSmallMemBlockInList(index);
	
	SLSmallMemBlock *pLeftBlock = (SLSmallMemBlock *)SplitMemBlock((SLMemBlock *)pBlock, size);
	if(pLeftBlock != NULL)
		InsertSmallMemBlock(pLeftBlock);
	
	return (char *)pBlock + sizeof(SLMemBlock);
}

int IsTheLastBlockInSegment(SLMemBlock *pBlock)
{
	int i = ((unsigned long)pBlock + pBlock->ulBlockSize) % SEGMENT_SIZE;
	
	if(i == 0)
		return 1;
	else
		return 0;
}

SLSmallMemBlock *GetFirstSmallMemBlockInList(int index)
{
	SLSmallMemBlock *pBlock = gHeapManager.SmallMemBlockListArray[index];
	
	gHeapManager.SmallMemBlockListArray[index] = pBlock->pNext;
	
	if(pBlock->pNext != NULL)
		pBlock->pNext->pPrev = NULL;

	return pBlock;
}

void InsertFirstSmallMemBlockInList(int index, SLSmallMemBlock *pBlock)
{
	pBlock->pNext = gHeapManager.SmallMemBlockListArray[index];
	pBlock->pPrev = NULL;
	gHeapManager.SmallMemBlockListArray[index] = pBlock;

	if(pBlock->pNext != NULL)
		pBlock->pNext->pPrev = pBlock;
}

SLLargeMemBlock *GetLargeMemBlockInTreeNodeList(SLLargeMemBlock *pBlock)
{
	if(pBlock == NULL)
		return NULL;

	if(pBlock->pPrev != NULL)
		pBlock->pPrev->pNext = pBlock->pNext;

	if(pBlock->pNext != NULL)
		pBlock->pNext->pPrev = pBlock->pPrev;

	return pBlock;
}

void InsertLargeMemBlockInTreeNodeList(SLLargeMemBlock *pPrevBlock, SLLargeMemBlock *pBlock)
{
	pBlock->pNext = pPrevBlock->pNext;
	pBlock->pPrev = pPrevBlock;

	pPrevBlock->pNext = pBlock;

	if(pBlock->pNext != NULL)
		pBlock->pNext->pPrev = pBlock;
}

SLBackupNode *GetFirstNodeInBackupList(void)
{
	SLBackupNode *pNode = gHeapManager.pBackupList;

	gHeapManager.pBackupList = pNode->pNext;
	
	if(pNode->pNext != NULL)
		pNode->pNext->pPrev = NULL;

	return pNode;
}

void InsertFirstNodeInBackupList(SLBackupNode *pNode)
{
	pNode->pNext = gHeapManager.pBackupList;
	pNode->pPrev = NULL;

	gHeapManager.pBackupList = pNode;

	if(pNode->pNext)
		pNode->pNext->pPrev = pNode;
}

void free(void *ptr)
{
	if(ptr == NULL)
		return;

	SLMemBlock *pBlock = (SLMemBlock *)((char *)ptr - sizeof(SLMemBlock));
	if(pBlock->FromMmapBit == 1)
	{
		munmap((void*)pBlock, pBlock->ulBlockSize);
		return;
	}

	SLMemBlock *pNewBlock = pBlock;
	unsigned long new_block_size = pBlock->ulBlockSize;

	if(pBlock->PrevBlkInUseBit == 0)
	{
		unsigned long prev_size = *((unsigned long *)pBlock - 1);
		SLMemBlock *pPrevBlock = (SLMemBlock *)((char *)pBlock - prev_size);

		RemoveMemBlockFromDataStructure(pPrevBlock);
		
		new_block_size += prev_size;
		pNewBlock = pPrevBlock;	
	}

	if(!IsTheLastBlockInSegment(pBlock))
	{
		SLMemBlock *pNextBlock = (SLMemBlock *)((char *)pBlock + pBlock->ulBlockSize);
		
		if(pNextBlock->CurBlkInUseBit == 0)
		{
			RemoveMemBlockFromDataStructure(pNextBlock);

			new_block_size += pNextBlock->ulBlockSize;
		}
	
	}

	if(new_block_size == SEGMENT_SIZE)
	{
		munmap((void*)pNewBlock, SEGMENT_SIZE);
		return;
	}

	pNewBlock->CurBlkInUseBit = 0;
	pNewBlock->PrevBlkInUseBit = 1;
	pNewBlock->FromMmapBit = 0;
	pNewBlock->ulBlockSize = new_block_size;

	SLMemBlock *pNewNextBlock = (SLMemBlock *)((char *)pNewBlock + new_block_size);

	*((unsigned long *)pNewNextBlock - 1) = new_block_size;

	if(!IsTheLastBlockInSegment(pNewBlock))
		pNewNextBlock->PrevBlkInUseBit = 0;

	InsertMemBlockIntoDataStructure(pNewBlock);	
}

void RemoveMemBlockFromDataStructure(SLMemBlock *pBlock)
{
	if(pBlock->ulBlockSize <= SMALL_MEM_BLOCK_MAXIMUM)
	{
		RemoveMemBlockFromSmallMemBlockList((SLSmallMemBlock *)pBlock);
		return;
	}

	if(pBlock->ulBlockSize < EXTRA_LARGE_MEM_BLOCK_MINIMUM)
	{
		RemoveMemBlockFromLargeMemBlockTree((SLLargeMemBlock *)pBlock);
		return;
	}

	RemoveMemBlockFromBackupList((SLBackupNode *)pBlock);
}

void RemoveMemBlockFromSmallMemBlockList(SLSmallMemBlock *pBlock)
{
	if(pBlock->pPrev == NULL)
	{
		int index = GetSmallMemBlockListArrayIndex(pBlock->MemBlockHead.ulBlockSize);
		
		GetFirstSmallMemBlockInList(index);
	}
	else
	{
		pBlock->pPrev->pNext = pBlock->pNext;

		if(pBlock->pNext != NULL)
			pBlock->pNext->pPrev = pBlock->pPrev;
	}
}

void RemoveMemBlockFromLargeMemBlockTree(SLLargeMemBlock *pBlock)
{
	if(pBlock->pPrev != NULL)
	{
		GetLargeMemBlockInTreeNodeList(pBlock);
		return;
	}

	int index = GetLargeMemBlockTreeArrayIndex(pBlock->MemBlockHead.ulBlockSize);

	if(pBlock->pNext != NULL)
	{
		pBlock->pNext->pParent = pBlock->pParent;
		pBlock->pNext->pLeftChild = pBlock->pLeftChild;
		pBlock->pNext->pRightChild = pBlock->pRightChild;
		pBlock->pNext->pPrev = NULL;

		if(pBlock->pLeftChild)
			pBlock->pLeftChild->pParent = pBlock->pNext;

		if(pBlock->pRightChild)
			pBlock->pRightChild->pParent = pBlock->pNext;

		UpdateParentNodeInLargeMemBlockTree(index, pBlock, pBlock->pNext);

		return;
	
}

	DeleteNodeInLargeMemBlockTree(index, pBlock);
}

void RemoveMemBlockFromBackupList(SLBackupNode *pBlock)
{
	if(pBlock->pPrev == NULL)
		GetFirstNodeInBackupList();
	else
	{
		pBlock->pPrev->pNext = pBlock->pNext;

		if(pBlock->pNext != NULL)
			pBlock->pNext->pPrev = pBlock->pPrev;
	}
}
