#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

#define MAX(x, y) (x) > (y) ? (x) : (y)

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define PACK(size, alloc) ((size) | (alloc))

/* get/put a word at a specific address p */
#define GET(p)          (*(unsigned int*)(p))
#define PUT(p, val)     (*(unsigned int*)(p) = (val))

/* Given header/footer compute the size and alloc */
#define GET_SIZE(p)     (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* bp points to the valid address of a block, not header */
#define HDRP(bp)        ((char*)(bp) - WSIZE)
#define FTRP(bp)        ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)   ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
#define PREV_BLKP(bp)   ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE)))

static char *heap_listp = NULL;

static void* extend_heap(size_t);

static void *coalesce(void *bp);

static void *find_fit(size_t asize);

static void place(void* bp, size_t asize);

int mm_init(void)
{
        if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)-1) {
                return -1;
        }

        PUT(heap_listp, 0);

        /* prologue block */
        PUT(heap_listp + 1 * WSIZE, PACK(DSIZE, 1));
        PUT(heap_listp + 2 * WSIZE, PACK(DSIZE, 1));

        /* epilogue block */
        PUT(heap_listp + 3 * WSIZE, PACK(0, 1));

        heap_listp += 2 * WSIZE;

        if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
                return -1;
        }

        return 0;
}

static void *extend_heap(size_t words)
{
        char *bp;
        size_t size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

        if ((long)((bp = mem_sbrk(size)) == (void*)-1)) {
                return NULL;
        }

        /* here is a little tricky, please pay attension */
        PUT(HDRP(bp), PACK(size, 0));  // HDRP(bp) is the old epilogue block
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // new epilogue block

        return coalesce(bp);
}

void mm_free(void* bp)
{
        size_t size = GET_SIZE(HDRP(bp));

        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        coalesce(bp);
}

static void *coalesce(void *bp)
{
        size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
        size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        size_t size = GET_SIZE(HDRP(bp));

        if (!prev_alloc  && !next_alloc) {
                return bp;
        } else if (!prev_alloc && next_alloc) {
                size += GET_SIZE(HDRP(PREV_BLKP(bp)));
                PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
                PUT(FTRP(bp), PACK(size, 0));
                bp = PREV_BLKP(bp);
        } else if (prev_alloc && !next_alloc) {
                size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
                PUT(HDRP(bp), PACK(size, 0));
                PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        } else {
                size += GET_SIZE(HDRP(PREV_BLKP(bp)));
                size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

                PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
                PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
                bp = PREV_BLKP(bp);
        }

        return bp;
}

void *mm_malloc(size_t size)
{
        size_t asize;
        size_t extendsize;
        char *bp;

        if (size == 0) {
                return NULL;
        }

        if (size <= DSIZE) {
                asize = 2 * DSIZE;
        } else {
                asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
        }

        if ((bp = (char*)find_fit(asize)) != NULL) {
                place(bp, asize);
                return bp;
        }

        extendsize = MAX(asize, CHUNKSIZE);
        if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
                return NULL;
        }

        place(bp, asize);
        return bp;
}

static void* find_fit(size_t asize)
{
        for (void *bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
                if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
                        return bp;
                }
        }
        return NULL;
}

static void place(void* bp, size_t asize)
{
        size_t csize = GET_SIZE(HDRP(bp));

        if ((csize - asize) >= 2 * DSIZE) {
                PUT(HDRP(bp), PACK(asize, 1));
                PUT(FTRP(bp), PACK(asize, 1));
                
                bp = NEXT_BLKP(bp);
                PUT(HDRP(bp), PACK(csize - asize, 0));
                PUT(FTRP(bp), PACK(csize - asize, 0));
        } else {
                PUT(HDRP(bp), PACK(csize, 1));
                PUT(FTRP(bp), PACK(csize, 1));
        }
}
