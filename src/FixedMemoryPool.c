#include "../include/FixedMemoryPool.h"



void *
FMP_METHOD(align_mem)(void * p_addr, const uint8_t * p_align) {

    void * p_aligned_addr = FMP_INC_ADDR(p_addr, *p_align);

    p_aligned_addr = FMP_ALIGN_MEM(p_aligned_addr, *p_align);

    return p_aligned_addr;
}


/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

void
FMP_METHOD(chain_blocks)(
    void ** pp_addr, const size_t * const p_bsize, const size_t * p_nblocks, const uint8_t * p_align) {

    /* Memory blocks */
    void **    pp_block;

    /* Save the first address */
    void *     p_mblock;

    /* Save the block state */
    uint16_t * p_state;

    /* Save the blocks size */
    uint32_t * p_bmeta;


    pp_block = pp_addr;

    p_mblock = (void *) pp_block;

    /* Run the loop until the address + the block + align size is out of bounds */
    for(; (FMP_SEG_ADDR( ((char *) pp_block + (*p_bsize) + (*p_align)), p_mblock, FMP_BSIZE)) ; ) {


        /* Save the alignment in the first memory block metadata */
        p_bmeta = (uint32_t *) pp_block;

        *p_bmeta = *p_align;
        /* Shift the size of the block's 2 positions ahead, to make space for the states */
        *p_bmeta <<= 0x2;

        /*
         *
         * The first byte of the pointer is to check if tbe block is taken or not
         * and the second byte is if the next block is taken or not, this way it can skip half
         * of the blocks - instead of doing O(n) checks, it does O(n/2)
         *
         */
        p_state = (uint16_t*) pp_block;
        *p_state = FMP_INITB; /* 0x0 */


        /* Increment the address to align the next block */
        *pp_block = (void *) ((char *) pp_block + (*p_bsize) + (*p_align));

        /* Align the address */
        *pp_block = (void *) FMP_METHOD(align_mem)(*pp_block, p_align);


        /* move to the next chunk of free memory */
        pp_block = *pp_block;

    }

    *pp_block = NULL;

    /* todo: DRY */
    p_state = (uint16_t*) pp_block;
    *p_state = FMP_INITB;
}


/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/


uintptr_t
FMP_METHOD(is_aligned)(const void * p_addr, const uint8_t * p_align) {

    uintptr_t addr;
    uint8_t   is_aligned;

    addr = (uintptr_t) p_addr;

    /* 1 if it is already aligned, 0 otherwise */
    is_aligned = !(addr % *p_align);

    return (uintptr_t) is_aligned * addr + (uintptr_t)FMP_METHOD(align_mem)((void *) addr, p_align) * !is_aligned;
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

/*
 * \desc    Allocates a fixed size memory pool
 *
 * \param1  The size of one memory block
 * \param2  The number of blocks to allocate
 * \param3  The alignment constraint
 *
 */
FixedMemoryPool_t *
FMP_METHOD(make)(const size_t * p_bsize, const size_t * p_nblocks, uint8_t * p_align) {

    //FMP_ASSERT("Invalid # blocks", *p_nblocks > 1)

    /* The size of one block of memory */
    size_t              block_size;
    size_t              i;

    /* Ptr to chain the blocks */
    void **             pp_block;
    FixedMemoryPool_t * p_pool;


    block_size = FMP_MAX(sizeof(void*), *p_bsize) + sizeof(void *);

    p_pool = malloc(sizeof(FixedMemoryPool_t));

    if(p_pool == NULL)
        return NULL;

    /* Initialize the lock and the condition */
    #if defined(FMP_MT)

        FMP_METHOD(init_mt)(p_pool);

    #endif

    /* check for overflow */


    /*
     * Add a word of additional memory to the beginning of each block
     * The block contains information if the block is occupied or not
     */

    p_pool->mem_block = malloc( (block_size) * (*p_nblocks) + (*p_align) );

    if(p_pool->mem_block == NULL) {
        free(p_pool);
        return NULL;
    }

    /*
     * Check if the address returned by malloc is already aligned
     * Else it gets aligned
     */
    p_pool->block = (void **) FMP_METHOD(is_aligned)(p_pool->mem_block, p_align);


    pp_block = p_pool->block;

    /* Chain all the p_bsize blocks together */
    FMP_METHOD(chain_blocks)(pp_block, &block_size, p_nblocks, p_align);

    return p_pool;
}


#if defined(FMP_MT)

void
FMP_METHOD(init_mt)(FixedMemoryPool_t * p_pool) {

    pthread_mutex_init(&(p_pool->FMP_lock), NULL);

    /* FMP_ASSERT("NULL lock", p_pool->FMP_lock != NULL) */

    pthread_cond_init(&(p_pool->FMP_cond));
}

void
FMP_METHOD(alloc_mt)(FixedMemoryPool_t * p_pool) {

    void **    pp_block;
    uint16_t * p_state;

    FMP_LOCK(p_pool)

    for(;;) {

    }

    /* Wait for a free address if the loop above failed */
    while(1) {

        FMP_TWAIT(p_pool)
        break;

    }

    FMP_UNLOCK(p_pool)

    return (void *) p_pool->free_addr

}


#endif


/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

void
FMP_METHOD(dealloc)(void ** pp_addr) {

    uint16_t * p_state;

    /* Point to the metadata block */
    p_state = (uint16_t *) ((char*)(*pp_addr) - 1);

    /* Change the block state to available */
    *p_state ^= FMP_AVLB;


}


/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

void *
FMP_METHOD(alloc)(FixedMemoryPool_t * p_pool) {

    if(0) {

    #if defined(FMP_MT)
    } else if(1) {

        return FMP_METHOD(alloc_mt)(p_pool);

    #endif
    } else if(1) {

        /* Declarations */
        void **     pp_block;
        uint16_t *  p_state;


        pp_block =  p_pool->block;

        /*
         * Loop until the current block is not NULL or the next block
         * in case the last block is an even block, since it skips one block
         */

        for( ; pp_block != NULL || *(pp_block) != NULL ; ) {

            p_state = (uint16_t *)(pp_block);

            /* Check if the block is available */
            if( (*p_state & 0x1) == 0) {

                /* Set it taken */
                *p_state ^= 0x1;

                return (void *) ( (char*) pp_block + sizeof(void*));
            }


            /* Check if the next block is available */
            if( (*p_state & 0x2) == 0 ) {

                p_state = (uint16_t *)(*pp_block);

                *p_state ^= 0x2;

                return (void*) ( (char*) *pp_block + sizeof(void*));
            }


            /* Skip the next block */
            pp_block = (void **) *( (void **) *(pp_block)); //(pp_block + (*p_bsize * 2)); /* Maybe I can do pp_block = *(*pp_block) ? */

        }

        return NULL;
    }

}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
