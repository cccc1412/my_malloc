//改到mallocfrombins了
#include <stdio.h>
#include <stdlib.h>
extern "C"{
#include "my_malloc.h"
}
#include <vector>
#include <algorithm>
#define LEN 10000
#define MALLOC_MAX 300000
#define THREAD_NUM 10

void print_chunks_in_use(std::vector<void*>malloc_vec) {
	printf("printing chunks malloced......\n");
	for(int i = 0; i<malloc_vec.size(); i++) {
		struct Chunk* chunk_ptr = (struct Chunk*)((char*)malloc_vec[i] - sizeof(Chunk));
		printf("chunk size:%d\n", chunk_ptr->ulChunkSize);
		printf("chunk CurBlkUseBit:%d\n", chunk_ptr->CurBlkInUseBit);
		printf("chunk PrevBlikUseBit:%d\n", chunk_ptr->PrevBlkInUseBit);
		printf("chunk FromMmapBit:%d\n", chunk_ptr->FromMmapBit);
		printf("\n");
	}
}

void *ThreadFun(void *args) {
	// long i = (long)args;
	// srand(i);
	void *p_array[LEN];
	int malloc_size[LEN];
	std::vector<void*> malloc_vec;
	for(int i = 0; i < LEN; i++) {
		malloc_size[i] = rand()%(MALLOC_MAX);
		//malloc_size[i] = 8;
	}
	
	for (int i = 0; i < LEN; i++) {
		int op = rand()%2;
		if(op == 0){//malloc
			void *p = MyMalloc(malloc_size[i]);
			if(p != NULL) {
				//printf("malloc......\n");
				malloc_vec.push_back(p);
				//printf("malloc succeed! %x, user size: %d\n", p, malloc_size[i]);
				// print_bins();
				// print_top();
				// print_chunks_in_use(malloc_vec);
			}
		} else{//free
			if(!malloc_vec.empty()) {
				//printf("free.........\n");
				std::random_shuffle(malloc_vec.begin(),malloc_vec.end());
				MyFree(malloc_vec.back());
				//printf("free %x\n", malloc_vec.back());
				malloc_vec.pop_back();
				// print_bins();
				// print_top();
				// print_chunks_in_use(malloc_vec);
			}
		}
		//print_bins();
	}
	printf("thread exit!\n");
    return 0;
}

void *AddBinTest(void *args) {
	std::vector<void*> malloc_vec;
	std::vector<void*> bin_vec;
	int size = SMALL_BIN_MAX + 1;
	for(int i = 0;i<20;i++) {
		void *p = MyMalloc(size);
		malloc_vec.push_back(p);
	}
	for (int i = 0;i<20; i+=2) {
		MyFree(malloc_vec[i]);
		bin_vec.push_back(malloc_vec[i]);
	}
	for(int i = 0;i<10;i+=2) {
		DeleteChunkInBin((struct Chunk*)((char*)bin_vec[i] - sizeof(struct Chunk)));
	}
}


int main() {
	pthread_t tid[THREAD_NUM];
	for(int i = 0; i < THREAD_NUM; i++) {
		pthread_create(&tid[i], NULL, ThreadFun, (void*)i);
	}
	for (int i = 0; i < THREAD_NUM; i++) {
		pthread_join(tid[i],NULL);
	}
    return 0;
}
