/**
 * @file malloc.c
 *
 * @author Dario Sneidermanis
 *
 * TODO: use a fenwick tree to optimize find_bin down to log n (if worth it)
 *       use a trie/balanced tree in big enough bins to optimize find_chunk
 *           down to log n
 *       support alignment
 *       free external alloc'ed memory
 *
 *
 * The main algorithm idea was taken from:
 *
 * http://gee.cs.oswego.edu/dl/html/malloc.html
 *
 *
 * The idea is to have chunks of memory delimited by headers and footers.
 * An allocated memory chunk looks like this:
 *
 *  _____________________
 * | STATUS (inuse)      |
 * |_____________________|
 * | SIZE (in bytes)     |
 * |_____________________|
 * |        DATA         |
 * |        ...          |
 * |_____________________|
 * | SIZE (in bytes)     |
 * |_____________________|
 *
 * And a free chunk looks like this:
 *
 *  ______________________
 * | STATUS (free)        |
 * |______________________|
 * | SIZE (in bytes)      |
 * |______________________|
 * | PREVIOUS FREE CHUNCK |
 * |______________________|
 * | NEXT FREE CHUNK      |
 * |______________________|
 * |      GARBAGE         |
 * |        ...           |
 * |______________________|
 * | SIZE (in bytes)      |
 * |______________________|
 *
 * Besides, we have an array of "bins", each with a list of free chunks of
 * a specific size (really a range of sizes)
 *
 */

#include "malloc.h"
#include <assert.h>
#include <string.h> /* for memset in calloc and memcpy in realloc */


/**
 * If the requested size is smaller than this, and a perfect fit isn't found,
 * try to use the chunk contiguous to the last allocation
 *
 * @see malloc
 */
#define MAX_SMALL_REQUEST 256


struct free_header {

    unsigned int status : 1;
    unsigned int size   : 31;

    struct free_header* next;
    struct free_header* prev;
};


struct inuse_header {

    unsigned int status : 1;
    unsigned int size   : 31;
};


struct footer {

    unsigned int size : 32;
};


#define FREE_STATUS  0

#define INUSE_STATUS 1


#define MIN_FREE_CHUNK_SIZE  ( sizeof( struct free_header ) + \
                               sizeof( struct footer ) )

#define MIN_INUSE_CHUNK_SIZE ( sizeof( struct inuse_header ) + \
                               sizeof( struct footer ) )


/**
 * Bin sizes from 16 bytes to 2 GB
 *
 * Note: bin_sizes[0] is never used, since a free_header must fit in a free
 *       chunk
 */

#define BIN_NUMBER  ( sizeof( bin_sizes ) / sizeof( bin_sizes[0] ) )

static const size_t bin_sizes[] = {

         8,    16,    24,    32,    40,    48,    56,    64,    72,    80,
        88,    96,   104,   112,   120,   128,   136,   144,   152,   160,
       168,   176,   184,   192,   200,   208,   216,   224,   232,   240,
       248,   256,   264,   272,   280,   288,   296,   304,   312,   320,
       328,   336,   344,   352,   360,   368,   376,   384,   392,   400,
       408,   416,   424,   432,   440,   448,   456,   464,   472,   480,
       488,   496,   504,   512,   576,   640,   768,  1024,  2048,  4096,
         0x2000,     0x4000,     0x8000,    0x10000,    0x20000,    0x40000,
        0x80000,   0x100000,   0x200000,   0x400000,   0x800000,  0x1000000,
      0x2000000,  0x4000000,  0x8000000, 0x10000000, 0x20000000, 0x40000000,
     0x80000000
};


struct memory_context {

    size_t free_memory;
    size_t last_chunk_size;

    void* ( *external_alloc )( size_t, size_t* );

    struct free_header* last_chunk;
    struct free_header  bins[ BIN_NUMBER ];
};


/**
 * Global current memory context
 */
static struct memory_context* context;


/**
 * Performs a binary search to find the first bin of size >= to a given size
 *
 * @param size  size of the memory (in bytes)
 *
 * @return a pointer to the bin
 */
static struct free_header* find_bin ( size_t size ) {

    size_t min_bin = 0, max_bin = BIN_NUMBER, curr_bin;

    assert( size < bin_sizes[ BIN_NUMBER - 1 ] );

    while ( min_bin + 1 != max_bin ) {

        curr_bin = ( min_bin + max_bin ) / 2;

        if ( bin_sizes[ curr_bin ] <= size )

            min_bin = curr_bin;
        else
            max_bin = curr_bin;
    }

    return context->bins + min_bin;
}


/**
 * Finds the first chunk of memory >= to a given size in a given bin
 *
 * @param bin   pointer to bin to explore
 * @param size  size of memory (in bytes)
 *
 * @return a pointer to the chunk's free header
 */
inline static struct free_header* find_chunk ( struct free_header* bin,
                                               size_t size )
{
    struct free_header* chunk = bin;

    do {

        chunk = chunk->next;

    } while ( chunk != bin && chunk->size < size );

    return chunk;
}


/**
 * Finds the first chunk of memory > to a given size in a given bin
 *
 * Note the use of > instead of >=, like in find_chunk. This version
 * is used to implement a tie breaking least-recently-used strategy,
 * which mantains (for some reason) low memory fragmentation
 *
 * @param bin   pointer to bin to explore
 * @param size  size of memory (in bytes)
 *
 * @return a pointer to the chunk's free header
 */
inline static struct free_header* find_upper_chunk ( struct free_header* bin,
                                                     size_t size )
{
    struct free_header* chunk = bin;

    do {

        chunk = chunk->next;

    } while ( chunk != bin && chunk->size <= size );

    return chunk;
}


/**
 * Adds a chunk of memory to the bins
 *
 * @param memory  free memory buffer
 * @param size    free memory buffer size (in bytes)
 */
static void add_free_chunk ( void* memory, size_t size ) {

    struct free_header* header;
    struct footer*      footer;

    assert( size >= MIN_FREE_CHUNK_SIZE );

    header = memory;

    header->status = FREE_STATUS;
    header->size   = size;

    header->next = find_upper_chunk( find_bin( size ), size );
    header->prev = header->next->prev;

    header->next->prev = header;
    header->prev->next = header;

    footer = (struct footer*)( (char*)memory + size ) - 1;

    footer->size = size;
}


/**
 * Splits a free chunk of memory in two chunks, the first of a specified size,
 * and the second goes back to the bins of free chunks
 *
 * @param header  pointer to the header of a free memory chunk
 * @param size    size of the desired chunk
 *
 * @return a pointer to the first chunk (which is set "inuse")
 */
static void* split_chunk ( struct free_header* header, size_t size ) {

    size_t left_size = header->size - size;

    if ( left_size < MIN_FREE_CHUNK_SIZE ) {

        size += left_size;
        left_size = 0;

    } else {

        context->last_chunk = (struct free_header*)( (char*)header + size );

        add_free_chunk( context->last_chunk, left_size );
    }

    header->status = INUSE_STATUS;
    header->size   = size;
    header->next   = NULL;
    header->prev   = NULL;

    ( (struct footer*)( (char*)header + size ) - 1 )->size = size;

    context->free_memory    -= size;
    context->last_chunk_size = left_size;

    return (struct inuse_header*)header + 1;
}


/**
 * Adds a new memory area for allocations to the current memory context
 *
 * The buffer should be < 2 GB
 *
 * @param memory  memory buffer
 * @param size    memory buffer size (in bytes)
 */
void add_malloc_buffer ( void* memory, size_t size ) {

    struct bound {

        struct inuse_header header;
        struct footer       footer;

    } *bound;

    if ( size < 2 * sizeof( struct bound ) + MIN_FREE_CHUNK_SIZE )
        return;

    bound  = memory;

    bound->header.status = INUSE_STATUS;
    bound->header.size   = sizeof( struct bound );
    bound->footer.size   = sizeof( struct bound );

    bound = (struct bound*)((char*)memory + size) - 1;

    bound->header.status = INUSE_STATUS;
    bound->header.size   = sizeof( struct bound );
    bound->footer.size   = sizeof( struct bound );

    memory = (struct bound*)memory + 1;
    size  -= 2 * sizeof( struct bound );

    add_free_chunk( memory, size );

    context->free_memory += size;
}


/**
 * Creates a new malloc context in the given memory buffer. Uses the remaining
 * memory for allocations
 *
 * Must be called before any malloc or free (unless a memory context has been
 * set manually)
 *
 * Size must be, at least, 180*sizeof( void* ) + 91*sizeof( size_t ), to let a
 * memory context fit
 *
 * @param memory  memory buffer
 * @param size    memory buffer size (in bytes)
 */
void init_malloc ( void* memory, size_t size ) {

    struct free_header* bin;

    assert( size >= sizeof( struct memory_context ) );

    context = memory;
    memory  = (struct memory_context*)memory + 1;
    size   -= sizeof( struct memory_context );

    context->free_memory     = 0;
    context->last_chunk_size = 0;
    context->external_alloc  = NULL;

    for ( bin = context->bins; (void*)bin < memory; bin++ ) {

        bin->size = sizeof( struct free_header );
        bin->next = bin;
        bin->prev = bin;
    }

    add_malloc_buffer( memory, size );
}


/**
 * Called when current free memory is not enough
 *
 * @param size  size of the requested memory
 *
 * @return a pointer to the allocated memory, or NULL if an error ocurred
 */
inline static void* out_of_memory ( size_t size ) {

    size_t total_size, new_size;
    void   *new_memory;

    if ( !context->external_alloc )
        return NULL;

    total_size = size + 2 * MIN_INUSE_CHUNK_SIZE;

    new_memory = context->external_alloc( total_size, &new_size );

    if ( !new_memory || new_size < total_size )
        return NULL;

    add_malloc_buffer( new_memory, new_size );

    size -= MIN_INUSE_CHUNK_SIZE;

    return malloc( size );
}


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
void* malloc ( size_t size ) {

    struct free_header *bin, *chunk;

    size += MIN_INUSE_CHUNK_SIZE;

    if ( size < MIN_FREE_CHUNK_SIZE )
        size  = MIN_FREE_CHUNK_SIZE;

    if ( size > context->free_memory )

        return out_of_memory( size );

    /* find first non-empty large enough bin */

    for ( bin = find_bin( size ); bin == bin->next; )

        if ( ++bin >= context->bins + BIN_NUMBER )

            return out_of_memory( size );

    /* find first large enough chunk */

    chunk = find_chunk( bin, size );

    if ( chunk == bin ) {

        for ( bin++; bin == bin->next; )

            if ( ++bin >= context->bins + BIN_NUMBER )

                return out_of_memory( size );

        chunk = bin->next;
    }

    /* heuristic to improve locality */

    if ( size <  chunk->size              &&
         size <= context->last_chunk_size &&
         size <= MAX_SMALL_REQUEST )
    {
        chunk = context->last_chunk;
    }

    /* take the chunk out of the bin */

    chunk->prev->next = chunk->next;
    chunk->next->prev = chunk->prev;

    return split_chunk( chunk, size );
}


/**
 * Allocates a chunk of memory large enough for @a count objects that are
 * @a size bytes each. The allocated memory is filled with 0s
 *
 * @param count  number of objects
 * @param size   size of each object (in bytes)
 *
 * @return a pointer to the allocated memory, or NULL if an error ocurred
 */
void* calloc ( size_t count, size_t size ) {

    size_t total_size = count * size;
    void*  memory     = malloc( total_size );

    if ( memory )
        memset( memory, 0, total_size );

    return memory;
}


/**
 * Returns a piece of allocated memory
 *
 * @param memory  pointer to the memory to be freed
 */
void free ( void* memory ) {

    struct free_header *header, *cont_header;
    struct footer *footer, *cont_footer;
    size_t size;

    if ( memory == NULL )
        return;

    header = (struct free_header*)( (struct inuse_header*)memory - 1 );
    footer = (struct footer*)( (char*)header + header->size ) - 1;
    size   = header->size;

    /* Do not try to free the context */

    assert( (char*)header + header->size <= (char*)context ||
            (char*)header                >= (char*)( context + 1 ) );

    /* Check chunk invariants */

    assert( header->status == INUSE_STATUS );
    assert( header->size   == footer->size );

    /* Update context */

    context->free_memory += size;

    /* Try to join with previous chunk */

    cont_footer = (struct footer*)header - 1;
    cont_header = (struct free_header*)( (char*)header - cont_footer->size );

    if ( cont_header->status == FREE_STATUS ) {

        assert( cont_header->size == cont_footer->size );

        cont_header->prev->next = cont_header->next;
        cont_header->next->prev = cont_header->prev;

        size += cont_header->size;

        header = cont_header;
    }

    /* Try to join with next chunk */

    cont_header = (struct free_header*)( footer + 1 );
    cont_footer = (struct footer*)( (char*)footer + cont_header->size );

    if ( cont_header->status == FREE_STATUS ) {

        assert( cont_header->size == cont_footer->size );

        cont_header->prev->next = cont_header->next;
        cont_header->next->prev = cont_header->prev;

        size += cont_header->size;

        if ( context->last_chunk == cont_header ) {

            context->last_chunk_size = 0;
        }
    }

    add_free_chunk( header, size );
}


/**
 * Resizes a previouly malloc'ed chunk of memory to a given new size
 *
 * @param memory  pointer to the memory to be resized
 * @param size    new desired size
 *
 * @return a pointer to the new chunk (may be different from original), or
 *         NULL if an error ocurred
 */
void* realloc ( void* memory, size_t size ) {

    void*                new_memory;
    struct inuse_header* header;
    struct footer*       footer;
    struct free_header*  next_header;

    if ( !memory )
        return malloc( size );

    header = (struct inuse_header*)memory - 1;
    footer = (struct footer*)( (char*)header + header->size ) - 1;

    assert( header->status == INUSE_STATUS );
    assert( header->size   == footer->size );

    size += MIN_INUSE_CHUNK_SIZE;

    /* if fits in current chunk */

    if ( size <= header->size ) {

        size_t left_size = header->size - size;

        if ( left_size < MIN_FREE_CHUNK_SIZE )
            return memory;

        header->size = size;

        header = (struct inuse_header*)( (char*)header + size );

        header->status = INUSE_STATUS;
        header->size   = left_size;
        footer->size   = left_size;

        footer = (struct footer*)header - 1;

        footer->size = size;

        free( header + 1 );

        return memory;
    }

    next_header = (struct free_header*)( footer + 1 );

    /* if fits in current and (free) next chunk */

    if ( next_header->status == FREE_STATUS &&
         next_header->size + (size_t)header->size < size )
    {
        footer = (struct footer*)( (char*)footer + next_header->size );

        footer->size = header->size += next_header->size;

        next_header->next->prev = next_header->prev;
        next_header->prev->next = next_header->next;

        next_header->next = NULL;
        next_header->prev = NULL;

        context->free_memory    -= next_header->size;
        context->last_chunk_size = 0;

        return memory;
    }

    /* else use an entirely new chunk */

    size -= MIN_INUSE_CHUNK_SIZE;

    new_memory = malloc( size );

    if ( new_memory ) {

        memcpy( new_memory, memory, size );
        free( memory );
    }

    return new_memory;
}


/**
 * Checks the integrity of the memory context
 *
 * Useful to detect buffer overflows and other memory corruptions
 *
 * @return NULL if no error was found, or a pointer to the block where the
 *         first memory corruption was detected
 */
void* check_malloc ( void ) {

    struct free_header *bin, *block, *last;
    struct footer *footer;
    size_t free_memory = context->free_memory;

    for ( bin = context->bins; bin < context->bins + BIN_NUMBER; bin++ ) {

        if ( bin->status != FREE_STATUS ) {

            /* printf( "Error in context, bin %d\n", bin - context->bins ); */
            return bin;
        }

        if ( bin->size != sizeof( struct free_header ) ) {

            /* printf( "Error in context, bin %d\n", bin - context->bins ); */
            return bin;
        }

        last = bin;

        for ( block = bin->next; block != bin; block = block->next ) {

            if ( block->status != FREE_STATUS ) {

                /* printf( "Error in block header\n" ); */
                return block;
            }

            if ( block->prev != last ) {

                /* printf( "Error in block header\n" ); */
                return block;
            }

            footer = (struct footer*)( (char*)block + block->size ) - 1;

            if ( block->size != footer->size ) {

                /* printf( "Error in block footer\n" ); */
                return footer;
            }

            last = block;
            free_memory -= block->size;
        }
    }

    if ( free_memory ) {

        /* printf( "Error in context, free memory amount inconcistency\n" ); */
        return context;
    }

    return NULL;
}


/**
 * Get a pointer with all malloc data, including all data-structures containing
 * the free chunks of memory
 *
 * Usefull when several separate malloc contexts are needed
 *
 * @return pointer to current context
 */
void* get_malloc_context ( void ) {

    return context;
}


/**
 * Set the current context to a previously gotten context
 *
 * @param context pointer to a context
 */
void set_malloc_context ( void* new_context ) {

    context = new_context;
}


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
void set_external_alloc ( void* ( *allocator )( size_t , size_t* ) ) {

    context->external_alloc = allocator;
}

