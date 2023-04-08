/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team06",
    /* First member's full name */
    "Seunghun Shin",
    /* First member's email address */
    "seunghunshin1284@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros*/
#define WSIZE  4   /* Word and header/footer size (bytes) */
#define DSIZE  8   /* Double word size (bytes)*/
#define CHUNKSIZE  (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

// /* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

#define GET(p)  (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

// /* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p)  (GET(p) & 0x1)

#define HDRP(bp)  ((char *)(bp) - WSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


static char* heap_listp = NULL;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);

int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return 0;
    PUT(heap_listp, 0);                             /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    /* Prologue footer */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));        /* Epilogue header */
    heap_listp += (2*WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}


static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
        
    /* Initialize free block header/footer and the eplilogue header */
    
    PUT(HDRP(bp), PACK(size, 0));       /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));       /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coaresce if the previous block was free */
    return coalesce(bp);

}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {         /* Case 1 */
        return bp;
    }
    
    else if (prev_alloc && !next_alloc) {        /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {       /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}


static void *find_fit(size_t asize){
    // void *bp = heap_listp;
    void *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
            return bp;
    }

    // while (GET_SIZE(HDRP(bp)) > 0){
    //     if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
    //         return bp;
    //     }
    //     bp = NEXT_BLKP(bp);
    // }
    return NULL;
}

static void place(void *bp, size_t asize){
    // 1. bp에서 asize만큼, allocation block을 할당하기
    // int newsize = asize;

    size_t oldsize = GET_SIZE(HDRP(bp));
    // PUT(HDRP(bp), PACK(asize, 1));
    // PUT(HDRP(bp)+asize-WSIZE, PACK(asize, 1));

    // PUT(HDRP(bp+asize), PACK(oldsize-asize, 0));
    // PUT(FTRP(bp+asize), PACK(oldsize-asize, 0));
// bp = NEXT_BLKP(bp);
    if ((oldsize - asize) >= DSIZE*2){
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(oldsize-asize, 0));
    PUT(FTRP(bp), PACK(oldsize-asize, 0));
    }
    else{
        PUT(HDRP(bp), PACK(oldsize, 1));
        PUT(FTRP(bp), PACK(oldsize, 1));
    }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;   /* Adjusted block size */
    size_t extendsize;  /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;

}

/*
 * mm_free - Freeing a block does nothing.
 */

void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);

}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
// void *mm_realloc(void *ptr, size_t size)
// {
//     void *oldptr = ptr;
//     void *newptr;

//     size_t copySize;


//     if (!oldptr) {
//         newptr = mm_malloc(size);
//         if (!newptr) { return NULL; }
//         }  
//     else 
//         {
//         copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
//         if (copySize < size) {
//             newptr = malloc(size);
//             if (!newptr) { return NULL; }

//             memcpy(newptr, oldptr, copySize);

//             free(oldptr);
//         }
//         else {
//             newptr = oldptr;
//         }
//     }
//     return newptr;
// }

void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    copySize = GET_SIZE(HDRP(oldptr));

    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}