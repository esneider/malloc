/**
 * @file malloc.h
 *
 * @author Dario Sneidermanis
 */

#ifndef _MALLOC_H_
#define _MALLOC_H_


#include <stddef.h>


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
 * Allocs a chunk of memory of a specified size
 *
 * For more info on the algorithm idea, go to
 * http://gee.cs.oswego.edu/dl/html/malloc.html
 *
 * @param size  size of the memory trying to be allocated (in bytes)
 *
 * @return a pointer to the allocated memory, or NULL if an error ocurred
 */
void* malloc ( size_t size );


/**
 * Returns a piece of allocated memory
 *
 * @param memory  pointer to the memory to be freed
 */
void free ( void* memory );


/**
 * Checks the integrity of the memory context
 *
 * Useful to detect buffer overflows and other memory corruptions
 *
 * @return NULL if no error was found, or a pointer to the block where the
 *         first memory corruption was detected
 */
void* check_malloc ( void );


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
 * Get a pointer with all malloc data, including all data-structures containing
 * the free chunks of memory
 *
 * Usefull when several separate malloc contexts are needed
 *
 * @return pointer to current context
 */
void* get_malloc_context ( void );


/**
 * Set the current context to a previously gotten context
 *
 * @param new_context pointer to a context
 */
void set_malloc_context ( void* new_context );


/**
 * Set an external allocator. When malloc runs out of memory, the provided
 * allocation function will be called
 *
 * This function should receive a first parameter with the minimum size of
 * memory to be allocated, and a second parameter where the actual size of
 * the memory allocated will be saved
 * It should return a pointer to the allocated memory, or NULL if an error
 * ocurred
 *
 * If the provided allocator is NULL, no external allocator will be used
 *
 * @param allocator  allocation function
 */
void set_external_alloc ( void* ( *allocator )( size_t , size_t* ) );


#endif /* _MALLOC_H_ */

