/**
 * @file malloc.h
 *
 * @author Dario Sneidermanis
 */

#ifndef _MALLOC_H_
#define _MALLOC_H_


#include <stddef.h>


/**
 * Adds a new memory area for allocations to the current memory context
 *
 * The buffer should be < 2 GB
 *
 * @param memory  memory buffer
 * @param size    memory buffer size (in bytes)
 */
void add_malloc_buffer ( void* memory, size_t size );


/**
 * Creates a new malloc context in the given memory buffer. Uses the remaining
 * memory for allocations
 *
 * Must be called before any malloc or free (unless a memory context has been
 * set manually)
 *
 * @param memory  memory buffer
 * @param size    memory buffer size (in bytes)
 */
void init_malloc ( void* memory, size_t size );


/**
 * Checks the integrity of the memory context
 *
 * Useful to detect buffer overflows and double frees
 *
 * @return NULL if no error was found, or a pointer to the block where the
 *         first memory corruption was detected
 */
void* check_malloc ( void );


#endif /* _MALLOC_H_ */

