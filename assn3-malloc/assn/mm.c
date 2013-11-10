/*
 * This implementation replicates the implicit list implementation
 * provided in the textbook
 * "Computer Systems - A Programmer's Perspective"
 * Blocks are never coalesced or reused.
 * Realloc is implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "StreetFighters",
    /* First member's full name */
    "Syed Shareq Rabbani",
    /* First member's email address */
    "shareq.rabbani@mail.utoronto.ca",
    /* Second member's full name (leave blank if none) */
    "S M Nadir Hassan",
    /* Second member's email address (leave blank if none) */
    "nadir.hassan@mail.utoronto.ca"
};

/*************************************************************************
 * Function Definitions
*************************************************************************/
void add_to_free_list(void *bp);
void remove_free_block(void *bp);
void print_fl();

/*************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
*************************************************************************/
#define WSIZE       sizeof(void *)            /* word size (bytes) */
#define DSIZE       (2 * WSIZE)            /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<7)      /* initial heap size (bytes) */

#define MAX(x,y) ((x) > (y)?(x) :(y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(uintptr_t *)(p))
#define PUT(p,val)      (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Get the address of the previous and next free block */
#define LOCATION_PREV_FREE_BLKP(bp)	((char *)(bp))
#define LOCATION_NEXT_FREE_BLKP(bp) ((char *)(bp) + WSIZE)

#define GET_NEXT_FREE_BLK(bp) GET((uintptr_t)(LOCATION_NEXT_FREE_BLKP(bp)))
#define GET_PREV_FREE_BLK(bp) GET((uintptr_t)(LOCATION_PREV_FREE_BLKP(bp)))


void* heap_listp = NULL;

/* ptr to the beginning of the free list */
void* free_listp = NULL;

/**********************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 **********************************************************/
 int mm_init(void)
 {
   if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
         return -1;
     PUT(heap_listp, 0);                         // alignment padding
     PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));   // prologue header
     PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));   // prologue footer
     PUT(heap_listp + (3 * WSIZE), PACK(0, 1));    // epilogue header
     heap_listp += DSIZE;

     return 0;
 }

/**********************************************************
 * coalesce
 * Covers the 4 cases discussed in the text:
 * - both neighbours are allocated
 * - the next block is available for coalescing
 * - the previous block is available for coalescing
 * - both neighbours are available for coalescing
 **********************************************************/
void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {       /* Case 1 */
        add_to_free_list(bp);	//add to the free list
    	return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_free_block(NEXT_BLKP(bp)); //remove the free block from the free list
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        add_to_free_list(bp);
        return (bp);
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        remove_free_block(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        add_to_free_list(bp);
        return (PREV_BLKP(bp));
    }

    else {            /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)))  +
            GET_SIZE(FTRP(NEXT_BLKP(bp)))  ;
        remove_free_block(PREV_BLKP(bp));
        remove_free_block(NEXT_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        add_to_free_list(bp);
        return (PREV_BLKP(bp));
    }
}

/**********************************************************
 * add_to_free_list
 * adds the free block to the free list
 **********************************************************/
void add_to_free_list(void *bp)
{
	if(free_listp == NULL)
	{
		//add the free block to the free list which is NULL
		free_listp = bp;

		//set the previous as 0 or (null/nothing)
		PUT(LOCATION_PREV_FREE_BLKP(bp),0);

		//set the next free block as 0 or (null/nothing)
		PUT(LOCATION_NEXT_FREE_BLKP(bp),0);

	}
	else
	{
		void *prev_free_block_head = free_listp;

		//Make the free block as the head of your free list
		free_listp = bp;

		//Set the next block of the new head as the previous head
		PUT(LOCATION_NEXT_FREE_BLKP(bp),(uintptr_t)prev_free_block_head);

		//Set the previous block of the new head as NULL
		PUT(LOCATION_PREV_FREE_BLKP(bp), 0);

		//Set the previous block of the previous head to the new head
		PUT(prev_free_block_head,(uintptr_t)free_listp);
	}
}

/**********************************************************
 * add_to_free_list
 * Removes one free block (pointed by bp) from the list
 * since it is being coalesced.
 **********************************************************/
void remove_free_block(void *bp)
{
	//get the next and previous free block pointers
	char *next_free_blk = GET_NEXT_FREE_BLK(bp);
	char *prev_free_blk = GET_PREV_FREE_BLK(bp);

	if(free_listp == bp)	//if free block is head of the free list
	{
		//check if there was only one block in the free list
		if(GET_NEXT_FREE_BLK(bp) == 0)
		{
			free_listp = NULL;
		}
		else
		{
			//set the head of the free list to the next block
			free_listp = GET_NEXT_FREE_BLK(bp);

			//set the previous ptr of the new head to 0 (nil/nothing)
			PUT(LOCATION_PREV_FREE_BLKP(free_listp),0);
		}
	}
	else
	{
		if(GET_NEXT_FREE_BLK(bp) == 0)
		{
			//set the next block of the previous block
			PUT(LOCATION_NEXT_FREE_BLKP(prev_free_blk),0);
		}
		else
		{
			printf("start 1\n");

			//set the next block of the previous block
			PUT(LOCATION_NEXT_FREE_BLKP(prev_free_blk),next_free_blk);

			printf("end 2\n");

			//set the previous block of the next block
			PUT(LOCATION_PREV_FREE_BLKP(next_free_blk),prev_free_blk);
		}
	}
}

/**********************************************************
 * extend_heap
 * Extend the heap by "words" words, maintaining alignment
 * requirements of course. Free the former epilogue block
 * and reallocate its new header
 **********************************************************/
void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignments */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ( (bp = mem_sbrk(size)) == (void *)-1 )
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));                // free block header
    PUT(FTRP(bp), PACK(size, 0));                // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));        // new epilogue header

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}


/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 **********************************************************/
void * find_fit(size_t asize)
{
    void *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            return bp;
        }
    }
    return NULL;
}

/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
void place(void* bp, size_t asize)
{
  /* Get the current block size */
  size_t bsize = GET_SIZE(HDRP(bp));

  PUT(HDRP(bp), PACK(bsize, 1));
  PUT(FTRP(bp), PACK(bsize, 1));
}

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *bp)
{
    if(bp == NULL){
      return;
    }
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    coalesce(bp);

    //print the previous and the next pointers
    printf("Get prev - %d\n",GET_PREV_FREE_BLK(bp));
    printf("Get next - %d\n",GET_NEXT_FREE_BLK(bp));
    printf("size of block is %d\n",GET_SIZE(HDRP(bp)));

}


/**********************************************************
 * mm_malloc
 * Allocate a block of size bytes.
 * The type of search is determined by find_fit
 * The decision of splitting the block, or not is determined
 *   in place(..)
 * If no block satisfies the request, the heap is extended
 **********************************************************/
void *mm_malloc(size_t size)
{
    size_t asize; /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char * bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);


	void* free_blk_ptr=free_listp;
	
	if(free_listp != NULL)
	{
		//printf("free list has something size of %d, and asize is %d, and size fields are %d \n",GET_SIZE(HDRP(free_listp)),size,asize);
		
		//traverse the free list and find a free block that is big enough to hold asize block
		while((GET_NEXT_FREE_BLK(free_blk_ptr)!=0) && (GET_SIZE(HDRP(free_blk_ptr)) < asize))
		{
			free_blk_ptr = GET_NEXT_FREE_BLK(free_blk_ptr);
		}
		
		//if no free block was found
		if((GET_NEXT_FREE_BLK(free_blk_ptr))==0)
		{
			void* new_heap_ptr = NULL;
			extendsize = MAX(asize, CHUNKSIZE);

			if ((new_heap_ptr = extend_heap(extendsize/WSIZE)) == NULL)
				return NULL;

			//take out the allocated block from the free list
			remove_free_block(new_heap_ptr);

			place(new_heap_ptr, asize);

			return new_heap_ptr;
		}

		//take out the allocated block from the free list
		remove_free_block(free_blk_ptr);

		//if it reaches this part it means that we have found a block the right size
		return free_blk_ptr;
	}
	else
	{
		/* No fit found. Get more memory and place the block */
		extendsize = MAX(asize, CHUNKSIZE);
		if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
		    return NULL;

		//take out the allocated block from the free list
		remove_free_block(bp);

		place(bp, asize);
		return bp;
    }
}

void print_fl()
{
	void* new_heap_ptr = free_listp;


	while((GET_NEXT_FREE_BLK(new_heap_ptr)!=0))
	{
		//printf("[%d][%d][%d]\n",GET_SIZE(HDRP(new_heap_ptr)),GET_PREV_FREE_BLK(new_heap_ptr),GET_NEXT_FREE_BLK(new_heap_ptr));
		new_heap_ptr = GET_NEXT_FREE_BLK(new_heap_ptr);
	}


}

/**********************************************************
 * mm_realloc
 * Implemented simply in terms of mm_malloc and mm_free
 *********************************************************/
void *mm_realloc(void *ptr, size_t size)
{
    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0){
      mm_free(ptr);
      return NULL;
    }
    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
      return (mm_malloc(size));

    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    /* Copy the old data. */
    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/**********************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 *********************************************************/
int mm_check(void){
	print_fl();

	return 1;
}
