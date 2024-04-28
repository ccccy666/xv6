#include "kernel/types.h"

//
#include "user/user.h"

//
#include "ummalloc.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define uint_SIZE (ALIGN(sizeof(uint)))


#define SIZE 4
#define ALIGN_SIZE 8
#define MIN_BLOCK 24
#define CHUNKSIZE (1<<12)

#define GET(p) (*(unsigned int *)(p))
#define PACK(size, alloc) ((size) | (alloc))
#define PUT(p, val) (*(unsigned int *)(p) = val)
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define UPDATE_PTR(p, ptr)	(*(unsigned long *)(p) = (unsigned long)(ptr))
#define UPDATE_PRED(p, ptr) (UPDATE_PTR((char *)(p), ptr))
#define UPDATE_SUCC(p, ptr) (UPDATE_PTR(((char *)(p)+(ALIGN_SIZE)), ptr))
#define GET_PRED(p) ((char *)(*(unsigned long *)(p)))
#define GET_SUCC(p)	((char *)(*(unsigned long *)(p + ALIGN_SIZE)))
#define NEXT_BLKP(tmp) ((char *)(tmp) + GET_SIZE(((char *)(tmp) - SIZE)))
#define PREV_BLKP(tmp) ((char *)(tmp) - GET_SIZE(((char *)(tmp) - ALIGN_SIZE)))

#define HEAD(tmp) ((char* )(tmp) - SIZE)
#define FOOT(tmp) ((char* )(tmp) + GET_SIZE(HEAD(tmp)) - ALIGN_SIZE)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
// uint MAX(uint x, uint y){
// 	return x > y ? x : y ;
// }
// uint PACK(uint size,uint alloc){
// 	return size | alloc;
// }
// uint GET(void *p){
// 	return *(unsigned int *)(p);
// }
// void PUT(void *p,uint val){
// 	*(unsigned int *)(p) = val;
// }
// void UPDATE_PTR(void *p,void *ptr){
// 	*(unsigned long *)(p) = (unsigned long )(ptr);
// }
// char * GET_PRED(void *p){
// 	return (char *)(*(unsigned long *)(p));
// }
// char * GET_SUCC(void *p){
// 	return (char *)(*(unsigned long *)(p + ALIGN_SIZE));
// }
// void UPDATE_PRED(void *p,void *ptr){
// 	UPDATE_PTR((char *)(p), ptr);
// }
// void UPDATE_SUCC(void *p,void *ptr){
// 	UPDATE_PTR((char *)(p) + (ALIGN_SIZE), ptr);
// }
// uint GET_SIZE(void *p){
// 	return GET(p) & ~0x7;
// }
// uint GET_ALLOC(void* p){
// 	return GET(p) & 0x1;
// }
// char* HEAD(void* tmp){
// 	return (char* )(tmp) - SIZE;
// }
// char* FOOT(void* tmp){
// 	return ((char* )(tmp) + GET_SIZE(HEAD(tmp)) - ALIGN_SIZE);
// }
// char* NEXT_BLKP(void* tmp){
// 	return (char *)(tmp) + GET_SIZE(HEAD(tmp));
// }
// char* PREV_BLKP(void* tmp){
// 	return (char *)(tmp) - GET_SIZE(((char *)(tmp) - ALIGN_SIZE));
// }

/* Free list pointer */
static char *free_list_head;
static char *free_list_tail;

void *find_fit(uint asize){
	void* p;
	void* q = 0;
	uint min_fit=0,size;

	for (p = GET_SUCC(free_list_head); p != free_list_tail; p = GET_SUCC(p)) {
    	size = GET_SIZE(HEAD(p));

		if (asize <= size){
			if(min_fit==0 || min_fit > size) {
				q = p;
				min_fit = size;
			}
      
    	}	
			
	}

	return q;
}

void *extend_heap(uint BYTES){
	char *tmp;
	char *ptr;
	uint size;
	
	size = ALIGN(BYTES);
	//size = (BYTES % 2) ? (BYTES+1) * SIZE : BYTES * SIZE;
	if ((long)(tmp = sbrk(size)) == -1)
		return 0;
	
	ptr = free_list_tail;
	PUT(HEAD(ptr), PACK(size, 0));
	PUT(FOOT(ptr), PACK(size, 0));
	free_list_tail = NEXT_BLKP(ptr);
	UPDATE_SUCC(ptr, free_list_tail);

	PUT(HEAD(free_list_tail), PACK(0, 1));
	UPDATE_PRED(free_list_tail, ptr);
	//PUT(free_list_tail+ALIGN_SIZE, PACK(0, 0));
	return combine(ptr);
	
}
// void *insert(void *p, void *ptr, uint size){
// 	PUT(HEAD(p), PACK(size, 0));		/* Free block header */
// 	PUT(FOOT(ptr), PACK(size, 0));		/* Free block footer */
// 	free_list_tail = NEXT_BLKP(ptr);	/* Update free list tailp */
// 	UPDATE_SUCC(ptr, free_list_tail);		/* Update free list */
// }
void *combine(void *ptr){
	void *prevptr = PREV_BLKP(ptr);
	void *nextptr = NEXT_BLKP(ptr);
	uint prev_alloc = GET_ALLOC(FOOT(PREV_BLKP(ptr)));
	uint next_alloc = GET_ALLOC(HEAD(NEXT_BLKP(ptr)));
	uint size = GET_SIZE(HEAD(ptr));
	
	
	if (prev_alloc && next_alloc) {
		return ptr;
	} else if (!prev_alloc && !next_alloc) {
		size += GET_SIZE(HEAD(prevptr)) + GET_SIZE(FOOT(nextptr));
		UPDATE_SUCC(prevptr, GET_SUCC(nextptr));
		UPDATE_PRED(GET_SUCC(nextptr), prevptr);
		PUT(HEAD(prevptr), PACK(size, 0));
		PUT(FOOT(nextptr), PACK(size, 0));
		ptr = prevptr;		
		
	} else if (!prev_alloc && next_alloc) {	
		size += GET_SIZE(FOOT(prevptr));
		UPDATE_SUCC(prevptr, GET_SUCC(ptr));
		UPDATE_PRED(GET_SUCC(ptr), prevptr);
		PUT(HEAD(prevptr), PACK(size, 0));
		PUT(FOOT(ptr), PACK(size, 0));
		ptr = prevptr;
	} else if (prev_alloc && !next_alloc) {
		size += GET_SIZE(HEAD(nextptr));
		UPDATE_SUCC(ptr, GET_SUCC(nextptr));
		UPDATE_PRED(GET_SUCC(nextptr), ptr);
		PUT(HEAD(ptr), PACK(size, 0));
		PUT(FOOT(nextptr), PACK(size, 0));
	}
    
	return ptr;
}
void divide(void *tmp, uint asize){
	uint size = GET_SIZE(HEAD(tmp));
	void *ptr;

	if (size - asize >= MIN_BLOCK) {
		PUT(HEAD(tmp), PACK(asize, 1));
		PUT(FOOT(tmp), PACK(asize, 1));
		ptr = NEXT_BLKP(tmp);
		UPDATE_SUCC(ptr, GET_SUCC(tmp));
		UPDATE_PRED(ptr, GET_PRED(tmp));
		UPDATE_SUCC(GET_PRED(ptr), ptr);
		UPDATE_PRED(GET_SUCC(ptr), ptr);
		
		PUT(HEAD(ptr), PACK(size-asize, 0));
		PUT(FOOT(ptr), PACK(size-asize, 0));
		
	} else {
		PUT(HEAD(tmp), PACK(size, 1));
		PUT(FOOT(tmp), PACK(size, 1));

		UPDATE_SUCC(GET_PRED(tmp), GET_SUCC(tmp));
		UPDATE_PRED(GET_SUCC(tmp), GET_PRED(tmp));
	}

}

int mm_init(void)
{
	if ((free_list_head = sbrk(4*SIZE+MIN_BLOCK)) == (void *)-1) {
		return -1;
	}
	PUT(free_list_head, PACK(0, 0));
	PUT(free_list_head+SIZE, PACK(24, 1));
	free_list_head += ALIGN_SIZE;
	free_list_tail = NEXT_BLKP(free_list_head);
	UPDATE_PRED(free_list_head, 0);
	UPDATE_SUCC(free_list_head, free_list_tail);
	PUT(free_list_head+(2*ALIGN_SIZE), PACK(24, 1));

	PUT(HEAD(free_list_tail), PACK(0, 1));
	UPDATE_PRED(free_list_tail, free_list_head);
	if (extend_heap(CHUNKSIZE) == 0) {
		return -1;
	}
    
    return 0;
}


void mm_free(void *ptr)
{
	char *p;
	uint size = GET_SIZE(HEAD(ptr));

	for (p = GET_SUCC(free_list_head); ; p = GET_SUCC(p)) {
		if (ptr < (void *)p) {
			PUT(HEAD(ptr), PACK(size, 0));
			PUT(FOOT(ptr), PACK(size, 0));
			UPDATE_SUCC(ptr, p);
			UPDATE_PRED(ptr, GET_PRED(p));
			UPDATE_SUCC(GET_PRED(p), ptr);
			UPDATE_PRED(p, ptr);
			break;
		}
	}

	combine(ptr);
}

void *mm_malloc(uint size)
{
	uint asize;
	char *tmp;

	if (size == 0)
		return 0;
  
	if (size <= 2*ALIGN_SIZE) {
		asize = MIN_BLOCK;
	} else {
		asize = ALIGN_SIZE * ((size + (ALIGN_SIZE) + (ALIGN_SIZE-1)) / ALIGN_SIZE);
	}

	if ((tmp = find_fit(asize)) == 0) {
		uint extendsize;
		extendsize = MAX(asize, CHUNKSIZE);
		if ((tmp = extend_heap(extendsize)) == 0)
			return 0;
	}

	divide(tmp, asize);

	return tmp; 	
}

void *mm_realloc(void *ptr, uint size)
{
	void *oldptr = ptr;
	void *nextptr;
	void *pred;
	void *succ;
	uint asize;
	
	if (ptr == 0) {
		return mm_malloc(size);
	} else if (size == 0) {
		mm_free(ptr);
		return 0;
	}

	if (size <= 2*ALIGN_SIZE) {
		asize = MIN_BLOCK;
	} else {
		asize = ALIGN_SIZE * ((size + (ALIGN_SIZE) + (ALIGN_SIZE-1)) / ALIGN_SIZE);
	}
	uint blockSize;
	blockSize = GET_SIZE(HEAD(ptr));

	
	if (asize == blockSize) {
		return ptr;
	} else if (asize < blockSize) {
		if (blockSize-asize >= MIN_BLOCK) {	
			PUT(HEAD(ptr), PACK(asize, 1));
			PUT(FOOT(ptr), PACK(asize, 1));
			nextptr = NEXT_BLKP(ptr);
			PUT(HEAD(nextptr), PACK(blockSize-asize, 0));
			PUT(FOOT(nextptr), PACK(blockSize-asize, 0));
			char *p;
			for (p = GET_SUCC(free_list_head); ; p = GET_SUCC(p)) {
				if (nextptr < (void *)p) {
					pred = GET_PRED(p);
					succ = p;
					UPDATE_PRED(nextptr, pred);
					UPDATE_SUCC(nextptr, succ);
					UPDATE_SUCC(pred, nextptr);
					UPDATE_PRED(p, nextptr);
					break;
				}
			}
		} 
		return ptr;
	} else {
		uint sizesum;
		void *newptr;
		nextptr = NEXT_BLKP(ptr);
		sizesum = GET_SIZE(HEAD(nextptr))+blockSize;
		if (GET_ALLOC(HEAD(nextptr)) || sizesum < asize) {
			newptr = find_fit(asize);
			if (newptr == 0) {
				uint extendsize;
				extendsize = MAX(asize, CHUNKSIZE);
				if ((newptr = extend_heap(extendsize)) == 0) {
					return 0;
				}
			}
			divide(newptr, asize);
			memcpy(newptr, oldptr, blockSize-2*SIZE);
			mm_free(oldptr);
			return newptr;
			
		} else {
			pred = GET_PRED(nextptr);
			succ = GET_SUCC(nextptr);
			if (sizesum-asize >= MIN_BLOCK) {
				PUT(HEAD(ptr), PACK(asize, 1));
				PUT(FOOT(ptr), PACK(asize, 1));
				nextptr = NEXT_BLKP(ptr);
				PUT(HEAD(nextptr), PACK(sizesum-asize, 0));
				PUT(FOOT(nextptr), PACK(sizesum-asize, 0));
				UPDATE_PRED(nextptr, pred);
				UPDATE_SUCC(nextptr, succ);
				UPDATE_SUCC(pred, nextptr);
				UPDATE_PRED(succ, nextptr);
			} else {
				PUT(HEAD(ptr), PACK(sizesum, 1));
				PUT(FOOT(ptr), PACK(sizesum, 1));
				UPDATE_SUCC(pred, succ);
				UPDATE_PRED(succ, pred);
			}
			return ptr;
		}
	}

}
