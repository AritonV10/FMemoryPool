/*
 *
 * MIT License
 *
 * Copyright (c) 2019 Ariton Vio
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */ 

#ifndef FMP_H
#define FMP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


/* Multithreaded defines */
#if defined(FMP_MT)

    #include <pthread.h>

    #define FMP_LOCK(pool) pthread_mutex_lock(&(pool->FMP_lock))

    #define FMP_UNLOCK(pool) pthread_mutex_unlock(&(pool->FMP_lock))

    #define FMP_TWAIT(pool) pthread_cond_wait(&(pool->FMP_cond), &(pool->FMP_lock))

#endif

#undef FMP_METHOD
#define FMP_METHOD(name) FixedMemoryPool_##name


/* Return the bigger value */
#undef FMP_MAX
#define FMP_MAX(lhs, rhs) ((lhs) > (rhs) ? (lhs) : (rhs))


/* Return an alligned address */
#undef FMP_ALIGN_MEM
#define FMP_ALIGN_MEM(addr, align) (void*)((uintptr_t) addr & ~(uintptr_t)(align - 1))

/* Increment an address */
#undef FMP_INC_ADDR
#define FMP_INC_ADDR(addr, offset) (void *) ( (char*) addr + (offset - 1) )


/* Return the last address of an allocated chunk of memory */
#define FMP_END_ADDR(addr, size) (uintptr_t) FMP_INC_ADDR(addr, size)


/* Check if the address is out of bounds */
#define FMP_SEG_ADDR(caddr, addr, size) (uintptr_t) (caddr) < FMP_END_ADDR((addr), (size))

#define FMP_BSIZE ((*p_bsize) * (*p_nblocks) + (*p_align - 1))


#undef FMP_ASSERT
#define FMP_ASSERT(msg, cond) do { \
        if(!cond) { \
            return NULL; \
        } \
    }while(0);

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

enum {

    /* Default States */
    FMP_INITB   = 0x00,

    /* Available */
    FMP_AVLB    = 0x01,

    /* Next Block Available */
    FMP_NBAVLB  = 0x10
};


/* [MetaData][Data] */
typedef struct {

     #if defined(FMP_MT)

        pthread_mutex_t FMP_lock;

        pthread_cond_t  FMP_cond;

        /* Pointer to the last deallocated memory address */
        uintptr_t * free_addr;

    #endif

    /* The blocks of memory */
    void **     block;

    /* The entire memory block */
    void *      mem_block;

} FixedMemoryPool_t;

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

extern FixedMemoryPool_t *
FMP_METHOD(make)(const size_t * p_bsize, const size_t * p_nblocks, uint8_t * p_align);

extern void
FMP_METHOD(dealloc)(void ** pp_addr);

void *
FMP_METHOD(alloc)(FixedMemoryPool_t * p_pool);

#endif
