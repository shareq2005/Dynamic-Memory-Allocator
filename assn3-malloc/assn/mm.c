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
void *extend_heap(size_t words);
void *get_segregated_list_ptr(size_t size);
int get_segregated_index(size_t size);
void * find_segregated_best_fit(size_t asize);
void * find_first_fit(void *free_list_index);

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

#define GET_NEXT_FREE_BLK(bp) GET((LOCATION_NEXT_FREE_BLKP(bp)))
#define GET_PREV_FREE_BLK(bp) GET((LOCATION_PREV_FREE_BLKP(bp)))


void* heap_listp = NULL;

/* ptr to the beginning of the free list */
void* free_listp = NULL;

/* ptr pointing to the segregated list */
void* segregated_list_ptr = NULL;
/**********************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 **********************************************************/
int mm_init(void)
{
	//free_listp = NULL;
	if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
		return -1;
	PUT(heap_listp, 0);                         // alignment padding
	PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));   // prologue header
	PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));   // prologue footer
	PUT(heap_listp + (3 * WSIZE), PACK(0, 1));    // epilogue header
	heap_listp += DSIZE;
	free_listp = NULL;


	//increase heap size for storing the head pointers
	segregated_list_ptr = mm_malloc(136);
	printf("segregate list ptr is at %p\n",segregated_list_ptr);
	printf("heap list ptr is at %p\n",heap_listp);

	printf("get alloc segregated list block is %d\n", GET_ALLOC(HDRP(segregated_list_ptr)));
	

	//set the head pointers of the segregated list to 0 (NULL)
	int i;
	for(i = 0; i < 136; i++)
	{
		PUT(segregated_list_ptr+(i*WSIZE),0);
	}

	printf("get alloc segregated list block is %d\n", GET_ALLOC(HDRP(segregated_list_ptr)));

	return 0;
}

print_ptr(void *bp){
	if(bp!=NULL){
		printf("prev is %d, size is %d, next is %d\n",GET_PREV_FREE_BLK(bp),GET_SIZE(HDRP(bp)),GET_NEXT_FREE_BLK(bp));
	}else
	{
		printf("ptr is NULL\n");
	}
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
	printf("IN COALESCE\n");
	printf("coalescing block ptr %p\n",bp);
	printf("size of prev blk hdr is %d\n",GET_SIZE(HDRP(PREV_BLKP(bp))));
	printf("size of prev blk ftr is %d\n",GET_SIZE(FTRP(PREV_BLKP(bp))));
	
	printf("next blk is %p\n",NEXT_BLKP(bp));
	printf("prev blk is %p\n",PREV_BLKP(bp));

	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	//printf("sizeof size_t %08p\n",sizeof(size_t));

	if (prev_alloc && next_alloc) {       /* Case 1 */
		printf("case 1\n");
		add_to_free_list(bp);	//add to the free list
		//print_ptr(bp);
		return bp;
	}
	else if (prev_alloc && !next_alloc) { /* Case 2 */

		printf("case 2\n");
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

		remove_free_block(NEXT_BLKP(bp)); //remove the free block from the free list
		PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
		add_to_free_list(bp);
		return (bp);
	}
	else if (!prev_alloc && next_alloc) { /* Case 3 */
		printf("case 3\n");
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));

		remove_free_block(PREV_BLKP(bp));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		add_to_free_list(PREV_BLKP(bp));
		//print_ptr(PREV_BLKP(bp));
		//print_ptr(bp);
		return (PREV_BLKP(bp));
	}
	else {            /* Case 4 */
		//printf("case 4 - size 1 = %d - size 2 = %d\n",GET_SIZE(HDRP(PREV_BLKP(bp))),GET_SIZE(FTRP(NEXT_BLKP(bp))));
		size += GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(FTRP(NEXT_BLKP(bp)));
		//printf("case 4 - prev blk ptr %p\n",PREV_BLKP(bp));
		//printf("case 4 - next blk ptr %p\n",NEXT_BLKP(bp));
		remove_free_block(PREV_BLKP(bp));
		remove_free_block(NEXT_BLKP(bp));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
		add_to_free_list(PREV_BLKP(bp));
		//print_ptr(bp);

		return (PREV_BLKP(bp));
	}
}


/**********************************************************
 * get_segregated_list_ptr 
 * calculates the address of the appropriate segeregated 
 * list ptr
 **********************************************************/
void *get_segregated_list_ptr(size_t size)
{
	void *bp;

	//get pointer of the appropriate segregated list
	bp = segregated_list_ptr + get_segregated_index(size);

	return bp;
}

/**********************************************************
 * get_segregated_index 
 * calculates the index of the segeregated list based on
 * its size
 **********************************************************/
int get_segregated_index(size_t size)
{
	//calculate the index of the appropriate segregated list
	if(size < 128)
		return (WSIZE*size);
	else if (size < 256)	//max block size 
		return (WSIZE*128);
	else if (size < 512)	//max block size 
		return (WSIZE*128) + (1*WSIZE);
	else if (size < 1024)	//max block size 
		return (WSIZE*128) + (2*WSIZE);
	else if (size < 2048)	//max block size 
		return (WSIZE*128) + (3*WSIZE);
	else if (size < 4096)	//max block size 
		return (WSIZE*128) + (4*WSIZE);
	else if (size < 8192)	//max block size 
		return (WSIZE*128) + (5*WSIZE);
	else	//for blocks of larger sizes
		return (WSIZE*128) + (6*WSIZE);

}
/**********************************************************
 * add_to_free_list
 * adds the free block to the free list
 **********************************************************/
void add_to_free_list(void *bp)
{
	printf("IN ADD_TO_FREE_LIST\n");
	printf("adding free block %p\n",bp);

	//get the size of the free block
	size_t size = GET(HDRP(bp));
	free_listp = get_segregated_list_ptr(size);

	//print_ptr(bp);
	if(GET(free_listp) == 0)
	{
		//add the free block to the free list which is NULL
		//free_listp = bp;
		PUT(free_listp,bp);

		//set the previous as 0 or (null/nothing)
		PUT(LOCATION_PREV_FREE_BLKP(bp),0);

		//set the next free block as 0 or (null/nothing)
		PUT(LOCATION_NEXT_FREE_BLKP(bp),0);

	}
	else
	{
		//Set the next block of the new head as the previous head
		PUT(LOCATION_NEXT_FREE_BLKP(bp),free_listp);

		//Set the previous block of the new head as NULL
		PUT(LOCATION_PREV_FREE_BLKP(bp), 0);

		//Set the previous block of the previous head to the new head
		PUT(LOCATION_PREV_FREE_BLKP(free_listp),bp);

		//free_listp = bp;
		PUT(free_listp,bp);
	}
}

/**********************************************************
 * add_to_free_list
 * Removes one free block (pointed by bp) from the list
 * since it is being coalesced.
 **********************************************************/
void remove_free_block(void *bp)
{
	printf("IN REMOVE_FREE_BLOCK\n");
	printf("previous free blk is %p\n",GET_PREV_FREE_BLK(bp));
	printf("next free blk is %p\n",GET_NEXT_FREE_BLK(bp));

	size_t size = GET_SIZE(HDRP(bp));	//get the size of the free block
	free_listp = get_segregated_list_ptr(size);

	if(!GET_PREV_FREE_BLK(bp) && !GET_NEXT_FREE_BLK(bp))	// case 1 - just one block in the free list
	{
		//free_listp = NULL;
		PUT(free_listp,0);
	}
	else if(!GET_PREV_FREE_BLK(bp) && GET_NEXT_FREE_BLK(bp))// case 2 - removing the head
	{
		//set the previous ptr of the new head to 0 (nil/nothing)
		//printf("location of prev free blk of next blk is %p\n",LOCATION_PREV_FREE_BLKP(GET_NEXT_FREE_BLK(bp)));

		PUT(LOCATION_PREV_FREE_BLKP(GET_NEXT_FREE_BLK(bp)),0);

		//set the head of the free list to the next block
		//free_listp = GET_NEXT_FREE_BLK(bp);
		PUT(free_listp,GET_NEXT_FREE_BLK(bp));
	}
	else if(GET_PREV_FREE_BLK(bp) && !GET_NEXT_FREE_BLK(bp))// case 3 - removing the last node in list
	{
		//set the next location of the previous block to 0
		PUT(LOCATION_NEXT_FREE_BLKP(GET_PREV_FREE_BLK(bp)),0);
	}
	else if(GET_PREV_FREE_BLK(bp) && GET_NEXT_FREE_BLK(bp))// case 4
	{
		//set the next location of the previous block
		PUT(LOCATION_NEXT_FREE_BLKP(GET_PREV_FREE_BLK(bp)),GET_NEXT_FREE_BLK(bp));

		//set the previous location of the next block
		PUT(LOCATION_PREV_FREE_BLKP(GET_NEXT_FREE_BLK(bp)),GET_PREV_FREE_BLK(bp));
	};
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
	//return coalesce(bp);
	return bp;
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

	//check if your free list is empty
	if(free_listp == NULL)
		return NULL;


	if((GET_NEXT_FREE_BLK(free_listp) == 0) && (asize <= GET_SIZE(HDRP(free_listp))))
	{
		return free_listp;
	};

	for (bp = free_listp; GET_NEXT_FREE_BLK(bp) != 0; bp = GET_NEXT_FREE_BLK(bp))
	{
		if (/*!GET_ALLOC(HDRP(bp)) &&*/ (asize <= GET_SIZE(HDRP(bp))))
		{
			return bp;
		};
	}

	//for cases where there is only one block in the list
	if(asize <= GET_SIZE(HDRP(bp)))
	{
		return bp;
	};

	return NULL;
}

/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned	
 **********************************************************/
void * find_segregated_best_fit(size_t asize)
{
	//get the segregated index
	int segregated_index = get_segregated_index(asize);

	//traverse through each segregated list
	int i;
	for(i = segregated_index; i < 136; i++)
	{
		//get one segregated list
		void *bp;
		bp = segregated_list_ptr + segregated_index;	

		//get one free blk
		void * free_blk = find_first_fit(bp);

		//return if free block found
		if(free_blk != NULL)
			return free_blk;
		else
			continue;
	}

	//if no free blk is found
	return NULL;
}

/**********************************************************
 * find_first_fit
 * Traverse the free list, if a free block is found then 
 * return it
 * Return NULL if no free blocks are found
 **********************************************************/
void * find_first_fit(void *free_list_index)
{
	//check if your free list is empty
	if(GET(free_list_index) == 0)
		return NULL;

	//return the first element in the list
	void *bp = GET(free_list_index);
	return bp;
}

/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
void place(void* bp, size_t asize)
{
	/* Get the current block size */
	size_t bsize = GET_SIZE(HDRP(bp));

	/* size 32 is the minimum possible chunk to hold some data*/
	if((bsize-asize) >= 32)	//check if splitting is possible
	{
		/* first block - which will be allocated*/
		//header
		PUT(HDRP(bp),PACK(asize,1));

		//footer
		PUT(FTRP(bp),PACK(asize,1));

		/* second block - which will be freed*/
		//header
		PUT((bp+asize-WSIZE),PACK(bsize-asize,0));

		//footer
		PUT(FTRP(bp+asize),PACK(bsize-asize,0));

		//add the second block to the free list
		add_to_free_list(bp+asize);
	}
	else	//if splitting is not possible
	{
		printf("header bp is %p\n",HDRP(bp));
		printf("footer bp is %p\n",FTRP(bp));
		PUT(HDRP(bp), PACK(bsize, 1));
		PUT(FTRP(bp), PACK(bsize, 1));
	}
}

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *bp)
{
	printf("IN FREE\n");
	printf("freeing block ptr %p\n",bp);

	if(bp == NULL){
		return;
	}

	size_t size = GET_SIZE(HDRP(bp));
	PUT(HDRP(bp), PACK(size,0));
	PUT(FTRP(bp), PACK(size,0));
	//print_fl();
	//printf("FROM MM_FREE\n");
	coalesce(bp);

	print_fl();
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
	printf("IN MALLOC\n");
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

    /* Search the free list for a fit */
    if ((bp = find_segregated_best_fit(asize)) != NULL) {
    	//print_fl();
    	printf("free size found, bp is %p\n",bp);

    	//remove
    	remove_free_block(bp);
    	place(bp, asize);
        return bp;
    };

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
    {
        printf("OUT OF MEMOROY\n");
        return NULL;
    }


    place(bp, asize);
    if(segregated_list_ptr != NULL)
    //printf("segregated_list_ptr alloc is %d\n",GET_ALLOC(HDRP(segregated_list_ptr)));
	printf("prev blk is %p\n",PREV_BLKP(bp));

    printf("mallocked block is %p\n",bp);
    return bp;

}

void print_fl()
{
	void* new_heap_ptr = free_listp;
	if(new_heap_ptr!=NULL){
		printf("start\n");
		while((GET_NEXT_FREE_BLK(new_heap_ptr)!=0))
		{
			printf("[%ld][%ld][%ld]\n",GET_SIZE(HDRP(new_heap_ptr)),GET_PREV_FREE_BLK(new_heap_ptr),GET_NEXT_FREE_BLK(new_heap_ptr));

			new_heap_ptr = GET_NEXT_FREE_BLK(new_heap_ptr);
		}
		printf("[%ld][%ld][%ld]\n",GET_SIZE(HDRP(new_heap_ptr)),GET_PREV_FREE_BLK(new_heap_ptr),GET_NEXT_FREE_BLK(new_heap_ptr));
		printf("end\n");
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
	//print_fl();

	return 1;
}
