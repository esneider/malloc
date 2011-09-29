/**
 * @file malloc.h
 *
 * @author Dario Sneidermanis
 */

#ifndef _MALLOC_H_
#define _MALLOC_H_


/*
 * Needs size_t, NULL and assert()
 */
#include <stddef.h>
#include <assert.h>


/**
 * Initializes the given memory for malloc use.
 *
 * Must be called before any malloc or free.
 *
 * @param size    the size of the given memory (in bytes)
 * @param memory  chunk of memory to be used
 *
 */
void init_malloc ( size_t size, void* memory );


/**
 * Allocstes a chunk of memory of a given size.
 *
 * @param size  the size trying to be allocated (in bytes)
 *
 * @return a pointer to the allocated memory, or NULL if an error occurred
 *
 * For more info on the algorithm idea:
 * @see http://gee.cs.oswego.edu/dl/html/malloc.html
 *
 */
void* malloc ( size_t size );


/**
 * Frees the previously allocated space.
 *
 * @param data  a pointer to the memory to be freed
 *
 */
void free ( void* date );


/**
 * Print memory data to screen.
 * Intended for debugging.
 *
 */
void print_memory_data ( void );


#endif /* _MALLOC_H_ */

