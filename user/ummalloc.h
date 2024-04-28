extern int mm_init(void);
extern void *mm_malloc(uint size);
extern void mm_free(void *ptr);
extern void *mm_realloc(void *ptr, uint size);
extern void *extend_heap(uint words);
extern void *combine(void *bp);
extern void *find_fit(uint asize);
extern void divide(void *bp, uint asize);
